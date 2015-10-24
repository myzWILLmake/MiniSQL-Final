#ifndef BPLUSTREE_H
#define BPLUSTREE_H

#define CREATE_TREE 0
#define FETCH_TREE  1

#define KEY_NOT_FOUND -1

#define INSERT_SUCCESS   0
#define INSERT_DUPLICATE 1
#define INSERT_FAILURE   2

#define REMOVE_SUCCESS 0
#define REMOVE_UNFOUND 1

#include "KeyValue.hpp"
#include "CatalogManager.hpp"
#include "BufferManager.hpp"
#include <string>
#include <vector>
#include <map>

using namespace std;

class Node {
	friend class BPlusTree;

	int parent;
	int next;
	int prev;
	bool is_root;
	bool is_leaf;
	int key_num;
	int child_num; // the number in 'offsets'
	int block_num;
	vector<KeyValue> keys;
	vector<int> offsets;

	Node();// create a new node; will call BufferManager for a new block
	Node(char *p, int type);// starting address of the memory block storing the node information
};

class BPlusTree {

private:
	int max_key_num;// the maximal number of keys in a node
	string table;
	string attr;
	int type; // the type of key
	map<int, Node *> nodes;
	map<int, IndexBlock *> blocks;

	BufferManager bm;
	CatalogManager cm;

public:
	// initialize the tree
	// mode:
	// 		CREATE_TREE: create a new tree and correspoding file
	//		FETCH_TREE: fetch an existing tree from file
	BPlusTree(string table, string attr, int mode);

	// constructor for debugging
	BPlusTree(int max);

	// insert new key value into the tree
	// return INSERT_SUCCESS on success
	// return INSERT_DUPLICATE if duplicate keys are encountered
	int insert(KeyValue key, int offset);

	// remove the key from the tree
	// return a bool variable indicating success or failure
	int remove(KeyValue key);

	// build a new tree on a bunch of keys
	bool build(vector<pair<KeyValue, int> > init);

	// internally called in 'build' function
	bool recur_build(vector<pair<KeyValue, int> > init);

	// search for a specific key in the tree
	// return KEY_NOT_FOUND(-1) on failure
	// return offset number on success
	int search(KeyValue key);

	// search for keys within specific ranges
	vector<int> search(KeyValue lower, bool lowerIn, KeyValue upper, bool upperIn);

	// delete the whole tree from file
	void destroy();

	// print for debug
	void print();

	// draw for debug
	void draw();

private:
	// search the tree recursively
	int recur_search(int block_num, KeyValue key);

	// search the tree recursively
	void recur_search(int block_num, vector<int> &indices, KeyValue lower, bool lowerIn, KeyValue upper, bool upperIn);

	// insert the key recursively
	int recur_insert(int block_num, KeyValue key, int offset);

	// split the node
	void split(int block_num);

	// read in the node content from file
	// if error occurs, exit directly
	void fetch(int block_num);

	// write the node content back to buffer
	void write_back(int block_num);

	// set pinned the block of the node 
	void set_pin(int block_num);

	// set unpinned the block of the node
	void reset_pin(int block_num);

	// release the node from memory
	void release(int block_num);

	// release all unpinned node from memory
	void release_unpinned();

	// inner function for 'remove'
	int recur_remove(int block_num, KeyValue key);

	// redistribute a node with its sibling node
	void redistribute(int block_num);

	// delete a node from the file
	void recycle(int block_num);

	// find the minimal value with all children nodes of a given node
	KeyValue find_min(int block_num);

	// delete the block and its children
	void recur_destroy(int block_num);
};

#endif