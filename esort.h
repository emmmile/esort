#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include <algorithm>
#include <functional>
#include <map>
#include <cassert>
using namespace std;
using namespace boost;
using namespace boost::archive;

#define USE_HEAP 1

template<class A, class B, class Compare>
struct heap_entry : public pair<A, B> {
    Compare comp;

    heap_entry( A a, B b, Compare c ) 
    : pair<A, B>(a, b), comp(c) { }

    bool operator< (const heap_entry& another) {
        return comp(another.first, this->first); // this inverts the order
    }
};


template<class IArchive, class T, class Compare>
size_t esort_run ( IArchive& iar, Compare comp, size_t memory, vector<T>& data ) {
    size_t n = 0;

    data.reserve(memory);
    data.clear();

    T a;
    while ( n < memory ) {
        try {
            iar >> a;
        } catch( archive_exception& ex ) {
            break;
        }

        data.push_back(a);
        ++n;
    }

    sort(data.begin(), data.end() );

    return n;
}

template<class IArchive, class OArchive, class T, class Compare>
size_t esort_merge ( OArchive& oar, Compare comp, string& prefix, size_t& index ) {
    typedef heap_entry<T, size_t, Compare> entry;

    vector<ifstream*> istreams;
    vector<IArchive*> iarchives;
    vector<entry> data;

    T a;
    for ( size_t i = 0; i < index; ++i ) {
        ifstream* ifs = new ifstream(prefix + to_string(i), ios::in);
        IArchive* iar = new IArchive(*ifs);
        *iar >> a;

        data.push_back(entry(a, i, comp));
        istreams.push_back(ifs);
        iarchives.push_back(iar);
    }

#if USE_HEAP
    make_heap(data.begin(), data.end());
#else
    sort(data.begin(), data.end());
#endif

    size_t total = 0;
    while (data.size() > 0) {
#if USE_HEAP
        pop_heap(data.begin(), data.end());
#endif
        auto minimum = data.back();
        data.pop_back();

        size_t index = minimum.second;
        a = minimum.first;
        oar << a;
        ++total;

        try {
            *iarchives[index] >> a;
            data.push_back(entry(a, index, comp));
#if USE_HEAP
            push_heap(data.begin(), data.end());
#else
            // this insertion sort step is even better than inplace_merge!
            for ( auto it = data.rbegin(); (it + 1) != data.rend() && *it < *(it + 1); ++it )
                swap(*it, *(it + 1));
#endif
        } catch( archive_exception& ex ) {}
    }

    for ( size_t i = 0; i < index; ++i ) {
        delete iarchives[i];
        delete istreams[i];
    }

    return total;
}


template<class T, class Compare = less<T>, class IArchive = text_iarchive, class OArchive = text_oarchive>
size_t esort( const char* in, const char* out, Compare comp = Compare(), size_t memory = 1024 * 1024 ) {
	ifstream ifs(in, ios::in);
    IArchive iar(ifs);

    vector<T> data;
    cout << "Reading and sorting blocks in '" << filesystem::temp_directory_path().native() << "'..." << endl;

    size_t index = 0;
    string prefix = filesystem::temp_directory_path().native() + "/esort_run_";

    while ( true ) {
        esort_run<IArchive, T>(iar, comp, memory, data);

        if ( data.size() > 0 ) {
            ofstream rofs(prefix + to_string(index), ios::out);
            OArchive roar(rofs);
            for ( T a : data ) roar << a;
        } else {
            break;
        }

        ++index;
    }

    cout << "Sorted " << index << " blocks. Now merging..." << endl;

    ofstream ofs(out, ios::out);
    OArchive oar(ofs);
    return esort_merge<IArchive, OArchive, T>(oar, comp, prefix, index);
}