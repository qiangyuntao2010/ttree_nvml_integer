/*******************************************************************************
 * speedtest/speedtest.cc
 *
 * STX B+ Tree Speed Test Program v0.9
 * Copyright (C) 2008-2013 Timo Bingmann
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <cassert>

#include <set>
#include <ext/hash_set>
#include <tr1/unordered_set>
//#include <stx/btree_multiset.h>

#include <map>
#include <ext/hash_map>
#include <tr1/unordered_map>
#include <libpmem.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

//#include <stx/btree_multimap.h>

//charlie
#include "ttree.h"
#include "ttree_set.h"
#include "ttree_multimap.h"

using namespace std;

// *** Settings

/// starting number of items to insert
static const unsigned int minitems = 1024;
//static const unsigned int minitems = 1024000 * 64;
//static const unsigned int minitems = 1;

/// maximum number of items to insert
static const unsigned int maxitems = 1024;
//static const unsigned int maxitems = 512000;

static const int randseed = 34;

/// b+ tree slot range to test
static const int min_nodeslots = 4;
static const int max_nodeslots = 256;

/// Time is measured using gettimeofday()
static inline double timestamp()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 0.000001;
}

/// Traits used for the speed tests, BTREE_DEBUG is not defined.
/*template <int _innerslots, int _leafslots>
class btree_traits_speed : stx::btree_default_set_traits<unsigned int>
{
public:
    static const bool selfverify = false;
    static const bool debug = false;

    static const int leafslots = _innerslots;
    static const int innerslots = _leafslots;

    static const size_t binsearch_threshold = 256 * 1024 * 1024; // never
};*/

// -----------------------------------------------------------------------------

/// Test a generic set type with insertions
template <typename SetType>
class Test_Set_Insert
{
public:
    explicit Test_Set_Insert(unsigned int) { }

    void run(unsigned int items)
    {
        SetType set;

        srand(randseed);
        for (unsigned int i = 0; i < items; i++)
            set.insert(rand());
		 assert(set.size() == items);
    }
};

/// Test a generic set type with insert, find and delete sequences
template <typename SetType>
class Test_Set_InsertFindDelete
{
public:
    explicit Test_Set_InsertFindDelete(unsigned int) { }

    void run(unsigned int items)
    {
        SetType set;

        srand(randseed);
        for (unsigned int i = 0; i < items; i++)
            set.insert(rand());

        //assert(set.size() == items);

        srand(randseed);
        for (unsigned int i = 0; i < items; i++)
            set.find(rand());

        srand(randseed);
        for (unsigned int i = 0; i < items; i++)
            set.erase(set.find(rand()));

        assert(set.empty());
    }
};

/// Test a generic set type with insert, find and delete sequences
template <typename SetType>
class Test_Set_Find
{
public:
    SetType set;

    explicit Test_Set_Find(unsigned int items)
    {
        srand(randseed);
        for (unsigned int i = 0; i < items; i++)
            set.insert(rand());

        assert(set.size() == items);
    }

    void run(unsigned int items)
    {
        srand(randseed);
        for (unsigned int i = 0; i < items; i++)
            set.find(rand());
    }
};

/// Construct different set types for a generic test class
template <template <typename SetType> class TestClass>
class TestFactory_Set
{
public:
    /// Test the multiset red-black tree from STL
    typedef TestClass<std::multiset<unsigned int> > StdSet;

    /// Test the multiset hash from gcc's STL extensions
    typedef TestClass<__gnu_cxx::hash_multiset<unsigned int> > HashSet;

    /// Test the unordered_set from STL TR1
    typedef TestClass<std::tr1::unordered_multiset<unsigned int> > UnorderedSet;

    /// Test the B+ tree with a specific leaf/inner slots
 /*   template <int Slots>
    class BtreeSet
        : public TestClass<
              stx::btree_multiset<unsigned int, std::less<unsigned int>,
                                  btree_traits_speed<Slots, Slots> > >
    {
    public:
        explicit BtreeSet(unsigned int n)
            : TestClass<
                  stx::btree_multiset<unsigned int, std::less<unsigned int>,
                                      btree_traits_speed<Slots, Slots> > >(n)
        { }
    };*/

	//charlie Test the T tree
	class TtreeSet : public TestClass<stx::ttree_set<char*, std::less<unsigned int> > >
	{
	public:
		explicit TtreeSet(char*  n)
			: TestClass<stx::ttree_set<char*, std::less<unsigned int > > >(n)
        {

        }
	};
    /// Run tests on all set types
    void call_testrunner(std::ostream& os, unsigned int items);
};

// -----------------------------------------------------------------------------

/// Test a generic map type with insertions
template <typename MapType>
class Test_Map_Insert
{
public:
    explicit Test_Map_Insert(unsigned int) { }

    void run(unsigned int items)
    {
        MapType map; //map is StdMap, HashMap...

      //  srand(randseed);
        unsigned int r = 39;
        for (unsigned int i = 0; i < items; i++) {
            //fprintf(stdout,"%s : ENTER THE RUN CIRCULATON AND THE R VALUE IS %d\n",__func__,r);
           // unsigned int r = rand()%10000;
            r++;
            map.insert(std::make_pair(r, r));
        }

       // assert(map.size() == items);
    }
};

/// Test a generic map type with insert, find and delete sequences
template <typename MapType>
class Test_Map_InsertFindDelete
{
public:
    explicit Test_Map_InsertFindDelete(unsigned int) { }

    void run(unsigned int items)
    {
        MapType map;

        clock_t begin, insert_end, find_end, delete_end; 
        
        begin = clock();
        std::cout<<"time is: "<<(double)(begin)/CLOCKS_PER_SEC<<std::endl;
        srand(randseed);
        for (unsigned int i = 0; i < items; i++) {
            unsigned int r = rand();
            map.insert(std::make_pair(r, r));
        }
        insert_end = clock();
        std::cout<<"time is: "<<(double)(insert_end)/CLOCKS_PER_SEC<<std::endl;
        //assert(map.size() == items);

        srand(randseed);
        for (unsigned int i = 0; i < items; i++){
            map.find(rand());}
        find_end = clock();
        std::cout<<"time is: "<<(double)(find_end)/CLOCKS_PER_SEC<<std::endl;


        srand(randseed);
        for (unsigned int i = 0; i < items; i++){
            map.erase(rand());}
        delete_end = clock();
        std::cout<<"time is: "<<(double)(delete_end)/CLOCKS_PER_SEC<<std::endl;

        std::cout<<"insert latency : "<<(double)(insert_end - begin)/CLOCKS_PER_SEC<<std::endl; 
        std::cout<<"find latency : "<<(double)(find_end - insert_end)/CLOCKS_PER_SEC<<std::endl; 
        std::cout<<"delete latency : "<<(double)(delete_end - find_end)/CLOCKS_PER_SEC<<std::endl; 
        //assert(map.empty());
    }
};

/// Test a generic map type with insert, find and delete sequences
template <typename MapType>
class Test_Map_Find
{
public:
    MapType map;

    explicit Test_Map_Find(unsigned int items)
    {
        srand(randseed);
        char* buffer = (char*)malloc(16);
        memset(buffer,0,16);
        char* r = buffer;
        for (unsigned int i = 0; i < items; i++) {
            unsigned int u = rand();
            sprintf(buffer,"%d",u);
            map.insert(std::make_pair(r, r));
        }
        cout << "charlie tree with " << items << " has been built " << endl;
    //    assert(map.size() == items);
    }

    void run(unsigned int items)
    {
        srand(randseed);
	//	cout << " charlie start find " << endl;
        for (unsigned int i = 0; i < items; i++)
            map.find(rand());
	//	cout << " charlie end find " << endl;
    }
};

/// Construct different map types for a generic test class
template <template <typename MapType> class TestClass>
class TestFactory_Map
{
public:
    /// Test the multimap red-black tree from STL
    typedef TestClass<std::multimap<unsigned int, unsigned int> > StdMap;

    /// Test the multimap hash from gcc's STL extensions
    typedef TestClass<__gnu_cxx::hash_multimap<unsigned int, unsigned int> > HashMap;

    /// Test the unordered_map from STL TR1
    typedef TestClass<std::tr1::unordered_multimap<
                          unsigned int, unsigned int> > UnorderedMap;

    /// Test the B+ tree with a specific leaf/inner slots
   /* template <int Slots>
    class BtreeMap
        : public TestClass<
              stx::btree_multimap<unsigned int, unsigned int,
                                  std::less<unsigned int>,
                                  btree_traits_speed<Slots, Slots> > >
    {
    public:
        explicit BtreeMap(unsigned int n)
            : TestClass<
                  stx::btree_multimap<unsigned int, unsigned int,
                                      std::less<unsigned int>,
                                      btree_traits_speed<Slots, Slots> > >(n)
        { }
    };*/
	class TtreeMap: public TestClass<stx::ttree_multimap<unsigned int, unsigned int, std::less<unsigned int> > >
	{
	public:
		explicit TtreeMap(unsigned int n)
			:TestClass<
			 stx::ttree_multimap<unsigned int, unsigned int, std::less<unsigned int> > >(n)
        {}

    };

    /// Run tests on all map types
    void call_testrunner(std::ostream& os, unsigned int items);
};

// -----------------------------------------------------------------------------

unsigned int repeatuntil;

/// Repeat (short) tests until enough time elapsed and divide by the runs.
template <typename TestClass>
void testrunner_loop(std::ostream& os, unsigned int items)
{
    unsigned int runs = 0;
    double ts1, ts2;

    do
    {
        runs++;                  // count repetition of timed tests


        	//TestClass is STdMap, HashMap, Unordered Map, B+Map
            TestClass test(items); // initialize test structures

		//	cout << " charlie before timestamp() " << endl;
            ts1 = timestamp();

            fprintf(stdout,"IN THE FILE %s WITH  THE FUNCTION %s : THE RUNS TIMES IS %d \n",__FILE__,__func__,runs);
            
            //for (unsigned int totaltests = 0;
            //     totaltests <= repeatuntil; totaltests += items)
           // {
                fprintf(stdout,"ENTER THE FOR CIRCULATON!\n");
                test.run(items);        // run timed test procedure
             //   ++runs;
                fprintf(stdout,"IN THE FILE %s WITH  THE FUNCTION %s : THE RUNS TIMES IS %d \n",__FILE__,__func__,runs);
           // }

            ts2 = timestamp();
        

		

        // discard and repeat if test took less than one second.
       // if ((ts2 - ts1) < 1.0) repeatuntil *= 2;
    }while(0);
   // while ((ts2 - ts1) < 1.0); // NOLINT
  //  pmem_unmap(pmemaddr,mapped_len);
	std::cerr << "do " << items << " repeat " << (repeatuntil / items)
                  << " time " << (ts2 - ts1) << "\n";

    os << std::fixed << std::setprecision(10)
       << ((ts2 - ts1) / runs) << " " << std::flush;
}

// Template magic to emulate a for_each slots. These templates will roll-out
// btree instantiations for each of the Low-High leaf/inner slot numbers.
template <template <int Slots> class functional, int Low, int High>
class btree_range
{
public:
    void operator () (std::ostream& os, unsigned int items)
    {
        testrunner_loop<functional<Low> >(os, items);
        btree_range<functional, Low + 2, High>()(os, items);
    }
};

template <template <int Slots> class functional, int Low>
class btree_range<functional, Low, Low>
{
public:
    void operator () (std::ostream& os, unsigned int items)
    {
        testrunner_loop<functional<Low> >(os, items);
    }
};

template <template <typename Type> class TestClass>
void TestFactory_Set<TestClass>::call_testrunner(
    std::ostream& os, unsigned int items)
{
    os << items << " " << std::flush;
	cout << "TestFactory_Set call_testrunner: items is " << items << endl;
#if 1
	cout << "StdSet " << endl;
    testrunner_loop<StdSet>(os, items);
	cout << "HashSet " << endl;
    testrunner_loop<HashSet>(os, items);
	cout << "UnorderedSet " << endl;
    testrunner_loop<UnorderedSet>(os, items);
#endif
	cout << "TTreeSet " << endl;
	testrunner_loop<TtreeSet>(os, items);
#if 0
	cout << "BtreeSet " << endl;
   // btree_range<BtreeSet, min_nodeslots, max_nodeslots>()(os, items);
    // just pick a few node sizes for quicker tests
    testrunner_loop<BtreeSet<4> >(os, items);
    for (int i = 6; i < 8; i += 2) os << "0 ";
    testrunner_loop<BtreeSet<8> >(os, items);
    for (int i = 10; i < 16; i += 2) os << "0 ";
    testrunner_loop<BtreeSet<16> >(os, items);
    for (int i = 18; i < 32; i += 2) os << "0 ";
    testrunner_loop<BtreeSet<32> >(os, items);
    for (int i = 34; i < 64; i += 2) os << "0 ";
    testrunner_loop<BtreeSet<64> >(os, items);
    for (int i = 66; i < 128; i += 2) os << "0 ";
    testrunner_loop<BtreeSet<128> >(os, items);
    for (int i = 130; i < 256; i += 2) os << "0 ";
    testrunner_loop<BtreeSet<256> >(os, items);
#endif

    os << "\n" << std::flush;
}

template <template <typename Type> class TestClass>
void TestFactory_Map<TestClass>::call_testrunner(
    std::ostream& os, unsigned int items)
{
    os << items << " " << std::flush;
#if 1
	cout << " StdMap " << endl;
    testrunner_loop<StdMap>(os, items);
#endif
#if 0
	cout << " HashMap " << endl;
    testrunner_loop<HashMap>(os, items);
	cout << " Unordered " << endl;
    testrunner_loop<UnorderedMap>(os, items);
#endif
/*	cout << " BtreeMap " <<endl;
    testrunner_loop<BtreeMap<64> >(os, items);*/
	cout << " TtreeMap " <<endl;
	testrunner_loop<TtreeMap>(os, items);
#if 1
  //  btree_range<BtreeMap, min_nodeslots, max_nodeslots>()(os, items);
#else
    // just pick a few node sizes for quicker tests
    testrunner_loop<BtreeMap<4> >(os, items);
    for (int i = 6; i < 8; i += 2) os << "0 ";
    testrunner_loop<BtreeMap<8> >(os, items);
    for (int i = 10; i < 16; i += 2) os << "0 ";
    testrunner_loop<BtreeMap<16> >(os, items);
    for (int i = 18; i < 32; i += 2) os << "0 ";
    testrunner_loop<BtreeMap<32> >(os, items);
    for (int i = 34; i < 64; i += 2) os << "0 ";
    testrunner_loop<BtreeMap<64> >(os, items);
    for (int i = 66; i < 128; i += 2) os << "0 ";
    testrunner_loop<BtreeMap<128> >(os, items);
    for (int i = 130; i < 256; i += 2) os << "0 ";
    testrunner_loop<BtreeMap<256> >(os, items);
#endif

    os << "\n" << std::flush;
}

/// Speed test them!
int main()
{
#if 0 
    {   // Set - speed test only insertion
        std::ofsteam os("speed-set-insert.txt");

        repeatuntil = minitems;

        for (unsigned int items = minitems; items <= maxitems; items *= 2)
        {
            std::cerr << "set: insert " << items << "\n";
            TestFactory_Set<Test_Set_Insert>().call_testrunner(os, items);
        }
    }
#endif
#if 0
    {   // Set - speed test insert, find and delete
        std::ofstream os("speed-set-all.txt");

        repeatuntil = minitems;

        for (unsigned int items = minitems; items <= maxitems; items *= 2)
        {
            std::cerr << "set: insert, find, delete " << items << "\n";
            TestFactory_Set<Test_Set_InsertFindDelete>().call_testrunner(os, items);
        }
    }
#endif
#if 0
    {   // Set - speed test find only
        std::ofstream os("speed-set-find.txt");

        repeatuntil = minitems;

        for (unsigned int items = minitems; items <= maxitems; items *= 2)
        {
            std::cerr << "set: find " << items << "\n";
            TestFactory_Set<Test_Set_Find>().call_testrunner(os, items);
        }
    }
#endif
#if 0
    {   // Map - speed test only insertion
        std::ofstream os("speed-map-insert.txt");

        repeatuntil = minitems;

       // for (unsigned int items = minitems; items <= maxitems; items *= 2)
        unsigned int items = minitems;
       // {

            std::cout << "map: insert " << items << "\n";
            TestFactory_Map<Test_Map_Insert>().call_testrunner(os, items);
        
    }
#endif
#if 1
    {   // Map - speed test insert, find and delete
        std::ofstream os("speed-map-all.txt");

        unsigned int items = minitems;
        repeatuntil = minitems;
		//minitems = 512000
        //for (unsigned int items =  minitems; items <= maxitems; items *= 2)
        //{
            std::cerr << "map: insert, find, delete " << items << "\n";
            TestFactory_Map<Test_Map_InsertFindDelete>().call_testrunner(os, items);
        //}
    }
#endif
#if 0
    {   // Map - speed test find only
        std::ofstream os("speed-map-find.txt");

        unsigned int items = minitems;
        repeatuntil = minitems;

       // for (unsigned int items = minitems; items <= maxitems; items *= 2)
       // {
            std::cerr << "map: find " << items << "\n";
            TestFactory_Map<Test_Map_Find>().call_testrunner(os, items);
        //}
    }
#endif
    return 0;

}
/******************************************************************************/
