#include "esort.h"
#include "timer.h"
#include <array>
using namespace std;


class object {
public:
	int a;
    string b;

	object() {}

	object(int a, string& c) : a(a), b(c) {
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
    	out << o.a << " " << o.b << endl;
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

    string str = "this is an example";
    for ( size_t i = 0; i < n; ++i ) {
        random_shuffle(str.begin(), str.end());
        object a(random() % 1000, str);
        data.push_back(a);
    }

    cout << "Created " << n << " objects in " << t.elapsed() << "s." << endl;
}

template<class OArchive>
void write ( const char* out, vector<object> data ) {
    ofstream ofs(out, ios::out);
    OArchive oar(ofs);
    timer t;

    for ( size_t i = 0; i < data.size(); ++i ) oar << data[i];

    cout << "Written " << data.size() << " objects in " << t.elapsed() << "s." << endl;
    ofs.close();
}

// check if the file is really sorted
template<class T, class Compare = less<T>, class IArchive>
void sorted ( const char* in, Compare comp = Compare() ) {
    ifstream ifs(in, ios::in);
    IArchive iar(ifs);

    object a;
    object b;
    iar >> a;
    while ( true ) {
        try {
            iar >> b;
        } catch( archive_exception& ex ) {
            break;
        }

        assert( !comp(b, a) );
        a = b;
    }


    ifs.close();
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

    std::vector<object> data;

    create(n, data);
    write<archive::text_oarchive>(input, data);

    timer t;
    size_t total = esort<object, less<object>>(input, output, less<object>(), 256 * 1024);
    cout << "Externally sorted " << total << " objects in " << t.elapsed() << "s." << endl;
    //sorted<object>(output);
}


int main ( ) {
    external(10000000);
    memory(10000000);

	return 0;
}