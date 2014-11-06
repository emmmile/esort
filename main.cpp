#include "esort.h"
#include "timer.h"
#include <array>
using namespace std;


class object {
public:
	int a;
    int b;

	object() {}

	object(int a, int c) : a(a), b(c) {
	}

	template<class Archive>
    void serialize(Archive &ar, const unsigned int file_version) {
    	ar & a;
    	ar & b;
    }

    bool operator < ( const object& o ) const {
    	return a == o.a ? (b < o.b) : a < o.a;
    }

    friend ostream& operator << ( ostream& out, object& o ) {
    	out << o.a << " " << o.b;
        return out;
    }
};


// elminate serialization overhead at the cost of
// never being able to increase the version.
BOOST_CLASS_IMPLEMENTATION(object, boost::serialization::object_serializable);

// eliminate object tracking (even if serialized through a pointer)
// at the risk of a programming error creating duplicate objects.
BOOST_CLASS_TRACKING(object, boost::serialization::track_never);


void create( size_t n, vector<object>& data ) {
    timer t;

    //string str = "this is an example";
    for ( size_t i = 0; i < n; ++i ) {
        //random_shuffle(str.begin(), str.end());
        object a(random() % 100000000, random() % 100000000);
        data.push_back(a);
    }

    cout << "Created " << n << " objects in " << t.elapsed() << "s." << endl;
}

template<class OArchive>
void write ( const char* out, const vector<object>& data ) {
    ofstream ofs(out, ios::out);
    OArchive oar(ofs);
    timer t;

    for ( size_t i = 0; i < data.size(); ++i ) oar << data[i];

    cout << "Written " << data.size() << " objects in " << t.elapsed() << "s." << endl;
}

template<class IArchive>
void read ( const char* in, vector<object>& data ) {
    ifstream ifs(in, ios::in);
    IArchive iar(ifs);
    timer t;

    data.clear();

    object a;
    while ( true ) {
        try {
            iar >> a;
        } catch( archive_exception& ex ) {
            break;
        }

        data.push_back(a);
    }

    cout << "Read " << data.size() << " objects in " << t.elapsed() << "s." << endl;
}

void memory ( size_t n) {
    std::vector<object> data;
    create(n, data);

    timer t;
    sort(data.begin(), data.end());

    cout << "In-memory sorted " << n << " objects in " << t.elapsed() << "s." << endl;
}

void external ( size_t n ) {
    const char* input = "input.txt";
    const char* output = "output.txt";
    typedef archive::binary_oarchive OArchive;
    typedef archive::binary_iarchive IArchive;


    std::vector<object> data;

    create(n, data);
    write<OArchive>(input, data);

    timer t;
    size_t total = esort<object, less<object>, IArchive, OArchive>(input, output, less<object>(), 16 * 1024 * 1024);
    cout << "Externally sorted " << total << " objects in " << t.elapsed() << "s." << endl;
    
    read<IArchive>(output, data);
    assert( is_sorted(data.begin(), data.end()));
}


int main ( ) {
    external(200000000);
    //memory(1000);

	return 0;
}
