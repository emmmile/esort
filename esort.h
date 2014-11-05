#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include <algorithm>
#include <functional>
#include <cassert>
using namespace std;
using namespace boost;
using namespace boost::archive;

template<class T, class Compare>
struct heap_entry : public pair<T, size_t> {
    Compare comp;

    heap_entry( T f, size_t s, Compare c ) 
    : pair<T, size_t>(f,s), comp(c) { }

    bool operator< (const pair<T,size_t>& another) {
        // this is actually wrong! another comparator is needed (or at least ==)
        return !comp(this->first, another.first);
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
    typedef heap_entry<T, Compare> entry;
    vector<entry> data;
    vector<ifstream*> istreams;
    vector<IArchive*> iarchives;

    T a;
    for ( size_t i = 0; i < index; ++i ) {
        ifstream* ifs = new ifstream(prefix + to_string(i), ios::in);
        IArchive* iar = new IArchive(*ifs);
        *iar >> a;

        entry e( a, i, comp );
        data.push_back(e);
        istreams.push_back(ifs);
        iarchives.push_back(iar);
    }

    make_heap(data.begin(), data.end());

    size_t total = 0;
    while (data.size() > 0) {
        pop_heap(data.begin(), data.end());
        entry minimum = data.back();
        data.pop_back();

        oar << minimum.first;
        ++total;

        T a;
        size_t index = minimum.second;
        try {
            *iarchives[index] >> a;
            entry e( a, index, comp );
            data.push_back(e);
            push_heap(data.begin(), data.end());
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
    cout << "Reading and sorting blocks..." << endl;

    size_t index = 0;
    string prefix = filesystem::temp_directory_path().native() + "/esort_run_";

    while ( true ) {
        esort_run<IArchive, T>(iar, comp, memory, data);

        if ( data.size() > 0 ) {
            ofstream rofs(prefix + to_string(index), ios::out);
            text_oarchive roar(rofs);
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