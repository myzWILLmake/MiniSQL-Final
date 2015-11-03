#include "BPlusTree.hpp"
#include "CatalogManager.hpp"
#include "BufferManager.hpp"
#include "API.hpp"
#include <iostream>
#include <vector>
#include <map>
#include <utility>

using namespace std;

BPlusTree::BPlusTree(string table, string attr, int type, int mode)
{
	this->table = table;
	this->attr = attr;
    /*
	this->type = cm.getAttributeType(table, attr);
	int key_size;
	if (cm.getAttributeType(table, attr) > 0)
		key_size = cm.getAttributeType(table, attr);
	else if (cm.getAttributeType(table, attr) == -1)
		key_size = sizeof(int);
	else if (cm.getAttributeType(table, attr) == 0)
		key_size = sizeof(float);
     */
    this->type = type;
    int key_size;
    if (type > 0)
        key_size = type;
    else if (type == -1)
        key_size = sizeof(int);
    else if (type == 0)
        key_size = sizeof(float);
	else
	{
		cout << "Type Error when constructing BPlusTree." << endl;
		exit(0);
	}
	/*
    //DEBUG
    this->max_key_num = 5;
    */
	this->max_key_num = (0x1000 - sizeof(int) * 6 - sizeof(bool) * 2) / (key_size + sizeof(int)) - 1;
	
	if (mode == CREATE_TREE)
	{
		blocks[0] = bm->getIndexNewBlock(table, attr);
		set_pin(0);
		nodes[0] = new Node(0);
		nodes[0]->parent = -1;
		nodes[0]->next = -1;
		nodes[0]->prev = -1;
		nodes[0]->is_root = true;
		nodes[0]->is_leaf = true;
		nodes[0]->key_num = 0;
		nodes[0]->child_num = 0;
		nodes[0]->block_num = 0;
		write_back(0);
		reset_pin(0);
		release(0);
	}
	else if (mode == FETCH_TREE)
	{
		blocks[0] = bm->getIndexBlock(table, attr, 0);
        /*
		nodes[0] = new Node(blocks[0]->address, cm.getAttributeType(table, attr));
         */
        nodes[0] = new Node(blocks[0]->address, type);
    }
	else
	{
		cout << "Unexpected opening mode in BPlusTree constructor." << endl;
		exit(0);
	}
}

// constructor for debugging
BPlusTree::BPlusTree(int max)
{
	max_key_num = max;
}

int BPlusTree::insert(KeyValue key, int offset)
{
	if (search(key) != KEY_NOT_FOUND)
		return INSERT_DUPLICATE;
	int result = recur_insert(0, key, offset);
	release_unpinned();
	return result;
}

void BPlusTree::fetch(int block_num)
{
	if (nodes.find(block_num) == nodes.end())
	{
		// fetch block from BufferManager
		// Initialize the node
		blocks[block_num] = bm->getIndexBlock(table, attr, block_num);
        /*
		nodes[block_num] = new Node(blocks[block_num]->address, cm.getAttributeType(table, attr));
         */
        nodes[block_num] = new Node(blocks[block_num]->address, type);
		//DEBUG
		//  
		//  if (block_num == 0)
		//  {
		//  	Node *node = new Node();
		//  	nodes[block_num] = node;
		//  	node->child_num = -1;
		//  	node->is_root = true;
		//  	node->is_leaf = true;
		//  	node->key_num = 0;
		//  	node->parent = -1;
		//  	node->prev = -1;
		//  	node->next = -1;
		//  }
	}
}

void BPlusTree::write_back(int block_num)
{
	blocks[block_num]->dirty = true;
	char *p = blocks[block_num]->address;

	*((int *)p) = nodes[block_num]->parent;
	p += sizeof(int);
	*((int *)p) = nodes[block_num]->next;
	p += sizeof(int);
	*((int *)p) = nodes[block_num]->prev;
	p += sizeof(int);
	*((bool *)p) = nodes[block_num]->is_root;
	p += sizeof(bool);
	*((bool *)p) = nodes[block_num]->is_leaf;
	p += sizeof(bool);
	*((int *)p) = nodes[block_num]->key_num;
	p += sizeof(int);
	*((int *)p) = nodes[block_num]->child_num;
	p += sizeof(int);
	*((int *)p) = nodes[block_num]->block_num;
	p += sizeof(int);
	for (int i = 0; i < nodes[block_num]->key_num; i++)
	{
		nodes[block_num]->keys[i].output(p);
	}
	for (int i = 0; i < nodes[block_num]->key_num; i++)
	{
		*((int *)p) = nodes[block_num]->offsets[i];
		p += sizeof(int);
	}
    if (!nodes[block_num]->is_leaf)
    {
        *((int *)p) = nodes[block_num]->offsets.back();
        p += sizeof(int);
    }
}

void BPlusTree::set_pin(int block_num)
{
    if (blocks.find(block_num) != blocks.end())
        blocks[block_num]->pin = true;
}

void BPlusTree::reset_pin(int block_num)
{
    if (blocks.find(block_num) != blocks.end())
        blocks[block_num]->pin = false;
}

void BPlusTree::release(int block_num)
{
	// check its existence
	if (blocks.find(block_num) == blocks.end())
	{
		if (nodes[block_num] != NULL)
			delete nodes[block_num];
		// remove from nodes
		nodes.erase(block_num);
		// reset_pin the block
		reset_pin(block_num);
		// remove from blocks[]
		blocks.erase(block_num);
	}
	
}

void BPlusTree::release_unpinned()
{
	vector<int> indices;
	for (map<int, IndexBlock *>::iterator iter = blocks.begin(); iter != blocks.end(); iter++)
	{
		if (iter->second->pin == false)
		{
			indices.push_back(iter->first);
		}
	}
	for (vector<int>::iterator iter = indices.begin(); iter != indices.end(); iter++)
	{
		if (nodes[*iter])
			delete nodes[*iter];
		nodes.erase(*iter);
		blocks.erase(*iter);
	}
}

int BPlusTree::recur_insert(int block_num, KeyValue key, int offset)
{
	// search for the position
	fetch(block_num);
	set_pin(block_num);

	Node *cur_node = nodes[block_num];
	if (cur_node->is_leaf) // insert to this node
	{
		vector<KeyValue>::iterator keys_iter = cur_node->keys.begin();
		vector<int>::iterator offsets_iter = cur_node->offsets.begin();
		for (; keys_iter != cur_node->keys.end(); keys_iter++, offsets_iter++)
		{
			if (*keys_iter > key)
				break;
		}
		cur_node->keys.insert(keys_iter, key);
		cur_node->offsets.insert(offsets_iter, offset);
		cur_node->key_num += 1;
		if (cur_node->key_num <= max_key_num) // if the node is not full, then insert directly
		{
			write_back(block_num);
			reset_pin(block_num);
			return INSERT_SUCCESS;
		}
		else // if the node is full, split into two nodes
		{
			split(block_num); // update parent recursively
			return INSERT_SUCCESS;
		}
	}
	else
	{
		int i;
		for (i = 0; i < cur_node->key_num; i++)
		{
			if (cur_node->keys[i] > key)
			{
				int result = recur_insert(cur_node->offsets[i], key, offset);
				reset_pin(block_num);
				return result;
			}
		}
		int result = recur_insert(cur_node->offsets[i], key, offset);
		reset_pin(block_num);
		return result;
	}
	return INSERT_FAILURE;
}

void BPlusTree::split(int block_num)
{
	Node *cur_node = nodes[block_num];
	// create a new node
    IndexBlock *tmp = bm->getIndexNewBlock(table, attr);
    blocks[tmp->blockNo] = tmp;
	Node *new_node = new Node(tmp->blockNo);
	nodes[new_node->block_num] = new_node;
	set_pin(new_node->block_num);
	new_node->parent = cur_node->parent;
	new_node->next = cur_node->next;
	new_node->prev = cur_node->block_num;

	if (cur_node->next != -1)
	{
		fetch(cur_node->next);
		set_pin(cur_node->next);
		nodes[cur_node->next]->prev = new_node->block_num;
		write_back(cur_node->next);
		reset_pin(cur_node->next);
		release(cur_node->next);
	}

	new_node->is_root = cur_node->is_root;
	new_node->is_leaf = cur_node->is_leaf;
	cur_node->next = new_node->block_num;
	
	// move second half keys and offsets to new node
	vector<KeyValue>::iterator first_key = cur_node->keys.begin() + cur_node->key_num/2;
	vector<KeyValue>::iterator last_key = cur_node->keys.end();
	vector<KeyValue>::iterator key_pos = new_node->keys.begin();
	new_node->keys.insert(key_pos, first_key, last_key);
	cur_node->keys.erase(first_key, last_key);
	vector<int>::iterator first_offset = cur_node->offsets.begin() + cur_node->key_num/2;
	vector<int>::iterator last_offset = cur_node->offsets.end();
	vector<int>::iterator offset_pos = new_node->offsets.begin();
	new_node->offsets.insert(offset_pos, first_offset, last_offset);
	cur_node->offsets.erase(first_offset, last_offset);
	new_node->key_num = cur_node->key_num - cur_node->key_num/2;
	cur_node->key_num = cur_node->key_num/2;

	// if it is not a leaf node, update all new node's children
	if (!new_node->is_leaf)
	{
		cur_node->keys.pop_back();
		cur_node->key_num -= 1;
		vector<int>::iterator iter = new_node->offsets.begin();
		for (int i = 0; iter != new_node->offsets.end(); iter++, i++)
		{
			fetch(*iter);
			set_pin(*iter);
			nodes[*iter]->parent = new_node->block_num;
			nodes[*iter]->child_num = i;
			write_back(*iter);
			reset_pin(*iter);
			release(*iter);
		}
	}

	// if the node is root
	if (cur_node->is_root)
	{
        IndexBlock *tmp = bm->getIndexNewBlock(table, attr);
        blocks[tmp->blockNo] = tmp;
		Node *tmp_node = new Node(tmp->blockNo);
		nodes[tmp_node->block_num] = tmp_node;
		set_pin(tmp_node->block_num);

		// copy cur_node to tmp_node
		// keep cur_node as root node
		tmp_node->parent = 0;
		tmp_node->next = cur_node->next;
		tmp_node->prev = cur_node->prev;
		tmp_node->is_root = false;
		tmp_node->is_leaf = cur_node->is_leaf;
		tmp_node->key_num = cur_node->key_num;
		tmp_node->child_num = 0;
		tmp_node->keys.swap(cur_node->keys);
		tmp_node->offsets.swap(cur_node->offsets);
		if (!tmp_node->is_leaf)
		{
			vector<int>::iterator iter = tmp_node->offsets.begin();
			for (int i = 0; iter != tmp_node->offsets.end(); iter++, i++)
			{
				fetch(*iter);
				set_pin(*iter);
				nodes[*iter]->parent = tmp_node->block_num;
				nodes[*iter]->child_num = i;
				write_back(*iter);
				reset_pin(*iter);
				release(*iter);
			}
		}

		// reinitialize cur_node as new root
		cur_node->parent = -1;
		cur_node->next = -1;
		cur_node->prev = -1;
		cur_node->is_root = true;
		cur_node->is_leaf = false;
		cur_node->key_num = 1;
		cur_node->child_num = -1;
		vector<KeyValue>::iterator keys_iter = cur_node->keys.begin();
		vector<int>::iterator offsets_iter = cur_node->offsets.begin();
		cur_node->keys.insert(keys_iter, find_min(new_node->block_num));
		cur_node->offsets.insert(offsets_iter, new_node->block_num);
		// reinitialize offsets_iter for inserting elements to the head
		offsets_iter = cur_node->offsets.begin();
		cur_node->offsets.insert(offsets_iter, tmp_node->block_num);

		// update new_node
		new_node->child_num = 1;
		new_node->parent = 0;
		new_node->is_root = false;
		new_node->prev = tmp_node->block_num;

		write_back(tmp_node->block_num);
		write_back(new_node->block_num);
		write_back(cur_node->block_num);
		reset_pin(tmp_node->block_num);
		reset_pin(new_node->block_num);
		reset_pin(cur_node->block_num);
	}
	else // update non-root parent
	{
		fetch(cur_node->parent);
		set_pin(cur_node->parent);
		Node *prt_node = nodes[cur_node->parent];
		new_node->child_num = cur_node->child_num + 1;
		vector<KeyValue>::iterator keys_iter = prt_node->keys.begin() + cur_node->child_num;
		vector<int>::iterator offsets_iter = prt_node->offsets.begin() + (cur_node->child_num + 1);
		prt_node->keys.insert(keys_iter, find_min(new_node->block_num));
		prt_node->offsets.insert(offsets_iter, new_node->block_num);
		// update children's 'child_num' field
		vector<int>::iterator iter = prt_node->offsets.begin() + (cur_node->child_num + 2);
		for (int i = cur_node->child_num + 2; iter != prt_node->offsets.end(); iter++, i++)
		{
			fetch(*iter);
			set_pin(*iter);
			nodes[*iter]->child_num = i;
			write_back(*iter);
			reset_pin(*iter);
			release(*iter);
		}
		prt_node->key_num += 1;
		if (prt_node->key_num > max_key_num)
			split(prt_node->block_num);
		else
		{
			write_back(cur_node->parent);
			reset_pin(cur_node->parent);
		}
	}

	write_back(new_node->block_num);
	write_back(cur_node->block_num);
	reset_pin(new_node->block_num);
	reset_pin(cur_node->block_num);
}

int BPlusTree::remove(KeyValue key)
{
	if (search(key) == KEY_NOT_FOUND)
		return REMOVE_UNFOUND;
	int result = recur_remove(0, key);
	release_unpinned();
	return result;
}

int BPlusTree::recur_remove(int block_num, KeyValue key)
{
	// search for the position
	// if the node is more than half-full, then remove directly and update parent if necessary
	// if the node become less than half-full, then merge with sibling
	// update parent recursively

	fetch(block_num);
	set_pin(block_num);

	Node *cur_node = nodes[block_num];
	if (cur_node->is_leaf)
	{
		vector<KeyValue>::iterator keys_iter = cur_node->keys.begin();
		vector<int>::iterator offsets_iter = cur_node->offsets.begin();
		for (; keys_iter != cur_node->keys.end(); keys_iter++, offsets_iter++)
		{
			if (*keys_iter == key)
			{
				cur_node->keys.erase(keys_iter);
				cur_node->offsets.erase(offsets_iter);
				cur_node->key_num -= 1;
				break;
			}
		}
		if (cur_node->key_num * 2 >= max_key_num || cur_node->is_root)
		{
			write_back(block_num);
			reset_pin(block_num);
			return REMOVE_SUCCESS;
		}
		else
		{
			redistribute(block_num);
			return REMOVE_SUCCESS;
		}
	}
	else
	{
		int i;
		for (i = 0; i < cur_node->key_num; i++)
		{
			if (cur_node->keys[i] > key)
				break;
		}
		int result = recur_remove(cur_node->offsets[i], key);
        // if cur_node is NULL
        // the only situation is that the root node collapse
        if (nodes.find(block_num) == nodes.end())
        {
            fetch(0);
            cur_node = nodes[0];
        }
		for (i = 0; i < cur_node->key_num; i++)
		{
			if (cur_node->keys[i] > key)
				break;
		}
		if (i != 0 && !cur_node->is_leaf)
		{
			cur_node->keys[i - 1] = find_min(cur_node->offsets[i]);
		}
		reset_pin(block_num);
		return result;
	}
}

void BPlusTree::redistribute(int block_num)
{
	Node *cur_node = nodes[block_num];
	fetch(cur_node->parent);
	set_pin(cur_node->parent);
	Node *prt_node = nodes[cur_node->parent];

	// merge with left sibling or borrow keys from left sibling
	bool is_left_sibling;
	if (cur_node->child_num == 0)
		is_left_sibling = false;
	else
		is_left_sibling = true;

	// fetch sibling node
	Node *sbl_node;
	if (cur_node->child_num == 0)
	{
		fetch(prt_node->offsets[1]);
		set_pin(cur_node->parent);
		sbl_node = nodes[prt_node->offsets[1]];
	}
	else
	{
		fetch(prt_node->offsets[cur_node->child_num-1]);
		set_pin(prt_node->offsets[cur_node->child_num-1]);
		sbl_node = nodes[prt_node->offsets[cur_node->child_num-1]];
	}

	if (sbl_node->key_num + cur_node->key_num > max_key_num) // borrow keys from sibling node
	{	
		int copy_num = (sbl_node->key_num + cur_node->key_num - max_key_num) / 2;
		if (copy_num == 0)
			copy_num = 1;

		vector<KeyValue> tmp_keys;
		vector<KeyValue>::iterator keys_iter; 
		vector<int> tmp_offsets;
		vector<int>::iterator offsets_iter;
		
		if (!cur_node->is_leaf) // borrow keys from non-leaf node
		{
			if (is_left_sibling) 
			{
				keys_iter = sbl_node->keys.end() - (copy_num-1);
				tmp_keys.insert(tmp_keys.begin(), prt_node->keys[cur_node->child_num-1]);
				tmp_keys.insert(tmp_keys.begin(), keys_iter, sbl_node->keys.end());
				cur_node->keys.insert(cur_node->keys.begin(), tmp_keys.begin(), tmp_keys.end());
				sbl_node->keys.erase(sbl_node->keys.end() - copy_num, sbl_node->keys.end());

				offsets_iter = sbl_node->offsets.end() - copy_num;
				tmp_offsets.insert(tmp_offsets.begin(), offsets_iter, sbl_node->offsets.end());
				cur_node->offsets.insert(cur_node->offsets.begin(), tmp_offsets.begin(), tmp_offsets.end());
				sbl_node->offsets.erase(sbl_node->offsets.end() - copy_num, sbl_node->offsets.end());

				prt_node->keys[cur_node->child_num-1] = find_min(cur_node->block_num);

				cur_node->key_num += copy_num;
				sbl_node->key_num -= copy_num;

				offsets_iter = cur_node->offsets.begin();
				for (int i = 0; offsets_iter != cur_node->offsets.end(); offsets_iter++, i++)
				{
					fetch(*offsets_iter);
					set_pin(*offsets_iter);
					nodes[*offsets_iter]->child_num = i;
					write_back(*offsets_iter);
					reset_pin(*offsets_iter);
					release(*offsets_iter);
				}
			}
			else
			{
				keys_iter = sbl_node->keys.begin() + copy_num - 1;
				tmp_keys.insert(tmp_keys.begin(), prt_node->keys[cur_node->child_num]);
				tmp_keys.insert(tmp_keys.end(), sbl_node->keys.begin(), keys_iter);
				cur_node->keys.insert(cur_node->keys.end(), tmp_keys.begin(), tmp_keys.end());
				sbl_node->keys.erase(sbl_node->keys.begin(), keys_iter + 1);

				offsets_iter = sbl_node->offsets.begin() + copy_num;
				tmp_offsets.insert(tmp_offsets.begin(), sbl_node->offsets.begin(), offsets_iter);

				int i = cur_node->offsets.size();
				for (offsets_iter = tmp_offsets.begin(); offsets_iter != tmp_offsets.end(); offsets_iter++, i++)
				{
					fetch(*offsets_iter);
					set_pin(*offsets_iter);
					nodes[*offsets_iter]->child_num = i;
					write_back(*offsets_iter);
					reset_pin(*offsets_iter);
					release(*offsets_iter);
				}

				offsets_iter = sbl_node->offsets.begin() + copy_num;
				cur_node->offsets.insert(cur_node->offsets.end(), sbl_node->offsets.begin(), offsets_iter);
				sbl_node->offsets.erase(sbl_node->offsets.begin(), offsets_iter);

				prt_node->keys[cur_node->child_num] = find_min(sbl_node->block_num);

				cur_node->key_num += copy_num;
				sbl_node->key_num -= copy_num;

				offsets_iter = sbl_node->offsets.begin();
				for (int i = 0; offsets_iter != sbl_node->offsets.end(); offsets_iter++, i++)
				{
					fetch(*offsets_iter);
					set_pin(*offsets_iter);
					nodes[*offsets_iter]->child_num = i;
					write_back(*offsets_iter);
					reset_pin(*offsets_iter);
					release(*offsets_iter);
				}
			}
			for (offsets_iter = tmp_offsets.begin(); offsets_iter != tmp_offsets.end(); offsets_iter++)
			{
				fetch(*offsets_iter);
				set_pin(*offsets_iter);
				nodes[*offsets_iter]->parent = cur_node->block_num;
				write_back(*offsets_iter);
				reset_pin(*offsets_iter);
				release(*offsets_iter);
			}
		}
		else // borrow keys from leaf node
		{
			if (is_left_sibling)
			{
				keys_iter = sbl_node->keys.end() - copy_num;
				cur_node->keys.insert(cur_node->keys.begin(), keys_iter, sbl_node->keys.end());
				sbl_node->keys.erase(keys_iter, sbl_node->keys.end());

				offsets_iter = sbl_node->offsets.end() - copy_num;
				cur_node->offsets.insert(cur_node->offsets.begin(), offsets_iter, sbl_node->offsets.end());
				sbl_node->offsets.erase(offsets_iter, sbl_node->offsets.end());
				
				prt_node->keys[cur_node->child_num-1] = find_min(cur_node->block_num);

				cur_node->key_num += copy_num;
				sbl_node->key_num -= copy_num;
			}
			else if (!is_left_sibling)
			{
				keys_iter = sbl_node->keys.begin() + copy_num;
				cur_node->keys.insert(cur_node->keys.end(), sbl_node->keys.begin(), keys_iter);
				sbl_node->keys.erase(sbl_node->keys.begin(), keys_iter);

				offsets_iter = sbl_node->offsets.begin() + copy_num;
				cur_node->offsets.insert(cur_node->offsets.end(), sbl_node->offsets.begin(), offsets_iter);
				sbl_node->offsets.erase(sbl_node->offsets.begin(), offsets_iter);
				prt_node->keys[cur_node->child_num] = find_min(sbl_node->block_num);

				cur_node->key_num += copy_num;
				sbl_node->key_num -= copy_num;
			}
		}
        write_back(prt_node->block_num);
        write_back(sbl_node->block_num);
        write_back(cur_node->block_num);
        reset_pin(prt_node->block_num);
        reset_pin(sbl_node->block_num);
        reset_pin(cur_node->block_num);
	}
	else // merge with sibling node
	{
		// for non-leaf nodes merging with sibling node
		if (!cur_node->is_leaf)
		{
			// update all sibling node's children
			vector<int>::iterator offsets_iter = sbl_node->offsets.begin();
			for ( ; offsets_iter != sbl_node->offsets.end(); offsets_iter++)
			{
				fetch(*offsets_iter);
				set_pin(*offsets_iter);
				nodes[*offsets_iter]->parent = cur_node->block_num;
				write_back(*offsets_iter);
				reset_pin(*offsets_iter);
				release(*offsets_iter);
			}
			if (is_left_sibling)
			{
				cur_node->prev = sbl_node->prev;
				if (sbl_node->prev != -1)
				{
					fetch(sbl_node->prev);
					set_pin(sbl_node->prev);
					nodes[sbl_node->prev]->next = cur_node->block_num;
					write_back(sbl_node->prev);
					reset_pin(sbl_node->prev);
					release(sbl_node->prev);
				}
				cur_node->keys.insert(cur_node->keys.begin(), prt_node->keys[cur_node->child_num-1]);
				cur_node->keys.insert(cur_node->keys.begin(), sbl_node->keys.begin(), sbl_node->keys.end());
				cur_node->offsets.insert(cur_node->offsets.begin(), sbl_node->offsets.begin(), sbl_node->offsets.end());
				cur_node->key_num += sbl_node->key_num + 1;
				if (cur_node->child_num > 1)
				{
					prt_node->keys[cur_node->child_num - 2] = find_min(cur_node->block_num);
				}
				prt_node->keys.erase(prt_node->keys.begin() + (cur_node->child_num - 1));
				prt_node->offsets.erase(prt_node->offsets.begin() + (cur_node->child_num - 1));
				// parent node's key_num will be updated outside this block
				cur_node->child_num -= 1;
				
				// renumber children of prt_node behind cur_node
				offsets_iter = prt_node->offsets.begin() + cur_node->child_num + 1;
				for (int i = cur_node->child_num + 1; offsets_iter != prt_node->offsets.end(); offsets_iter++, i++)
				{
					fetch(*offsets_iter);
					set_pin(*offsets_iter);
					nodes[*offsets_iter]->child_num = i;
					write_back(*offsets_iter);
					reset_pin(*offsets_iter);
					release(*offsets_iter);
				}

				// renumber the cur_node's children
				int i = sbl_node->key_num + 1;
				offsets_iter = cur_node->offsets.begin() + i;
				for (; offsets_iter != cur_node->offsets.end(); offsets_iter++, i++)
				{
					fetch(*offsets_iter);
					set_pin(*offsets_iter);
					nodes[*offsets_iter]->child_num = i;
					write_back(*offsets_iter);
					reset_pin(*offsets_iter);
					release(*offsets_iter);
				}

			}
			else
			{
				cur_node->next = sbl_node->next;
				if (sbl_node->next != -1)
				{
					fetch(sbl_node->next);
					set_pin(sbl_node->next);
					nodes[sbl_node->next]->prev = cur_node->block_num;
					write_back(sbl_node->next);
					reset_pin(sbl_node->next);
					release(sbl_node->next);
				}
				// renumber sibling node's children in advance
				offsets_iter = sbl_node->offsets.begin();
				for (int i = cur_node->key_num + 1; offsets_iter != sbl_node->offsets.end(); offsets_iter++, i++)
				{
					fetch(*offsets_iter);
					set_pin(*offsets_iter);
					nodes[*offsets_iter]->child_num = i;
					write_back(*offsets_iter);
					reset_pin(*offsets_iter);
					release(*offsets_iter);
				}

				cur_node->keys.insert(cur_node->keys.end(), prt_node->keys[cur_node->child_num]);
				cur_node->keys.insert(cur_node->keys.end(), sbl_node->keys.begin(), sbl_node->keys.end());
				cur_node->offsets.insert(cur_node->offsets.end(), sbl_node->offsets.begin(), sbl_node->offsets.end());
				cur_node->key_num += sbl_node->key_num + 1;
				prt_node->keys.erase(prt_node->keys.begin());
				prt_node->offsets.erase(prt_node->offsets.begin() + 1);
				// parent node's key_num will be updated outside this block

				offsets_iter = prt_node->offsets.begin() + 1;
				for (int i = 1; offsets_iter != prt_node->offsets.end(); offsets_iter++, i++)
				{
					fetch(*offsets_iter);
					set_pin(*offsets_iter);
					nodes[*offsets_iter]->child_num = i;
					write_back(*offsets_iter);
					reset_pin(*offsets_iter);
					release(*offsets_iter);
				}
			}
		}
		else // for leaf nodes merging with sibling node
		{
			if (is_left_sibling)
			{
				cur_node->prev = sbl_node->prev;
				if (sbl_node->prev != -1)
				{
					fetch(sbl_node->prev);
					set_pin(sbl_node->prev);
					nodes[sbl_node->prev]->next = cur_node->block_num;
					write_back(sbl_node->prev);
					reset_pin(sbl_node->prev);
					release(sbl_node->prev);
				}
				cur_node->keys.insert(cur_node->keys.begin(), sbl_node->keys.begin(), sbl_node->keys.end());
				cur_node->offsets.insert(cur_node->offsets.begin(), sbl_node->offsets.begin(), sbl_node->offsets.end());
				cur_node->key_num += sbl_node->key_num;
				if (cur_node->child_num > 1)
					prt_node->keys[cur_node->child_num - 2] = find_min(cur_node->block_num);
				prt_node->keys.erase(prt_node->keys.begin() + (cur_node->child_num - 1));
				prt_node->offsets.erase(prt_node->offsets.begin() + (cur_node->child_num - 1));
				cur_node->child_num -= 1;
				// parent node's key_num will be updated outside this block

				// renumber children of prt_node behind cur_node
				vector<int>::iterator offsets_iter = prt_node->offsets.begin() + cur_node->child_num + 1;
				for (int i = cur_node->child_num + 1; offsets_iter != prt_node->offsets.end(); offsets_iter++, i++)
				{
					fetch(*offsets_iter);
					set_pin(*offsets_iter);
					nodes[*offsets_iter]->child_num = i;
					write_back(*offsets_iter);
					reset_pin(*offsets_iter);
					release(*offsets_iter);
				}
			}
			else
			{
				cur_node->next = sbl_node->next;
				if (sbl_node->next != -1)
				{
					fetch(sbl_node->next);
					set_pin(sbl_node->next);
					nodes[sbl_node->next]->prev = cur_node->block_num;
					write_back(sbl_node->next);
					reset_pin(sbl_node->next);
					release(sbl_node->next);
				}
				cur_node->keys.insert(cur_node->keys.end(), sbl_node->keys.begin(), sbl_node->keys.end());
				cur_node->offsets.insert(cur_node->offsets.end(), sbl_node->offsets.begin(), sbl_node->offsets.end());
				cur_node->key_num += sbl_node->key_num;
				prt_node->keys.erase(prt_node->keys.begin() + cur_node->child_num);
				prt_node->offsets.erase(prt_node->offsets.begin() + (cur_node->child_num + 1));

				vector<int>::iterator offsets_iter = prt_node->offsets.begin() + cur_node->child_num + 1;
				for (int i = cur_node->child_num + 1; offsets_iter != prt_node->offsets.end(); offsets_iter++, i++)
				{
					fetch(*offsets_iter);
					set_pin(*offsets_iter);
					nodes[*offsets_iter]->child_num = i;
					write_back(*offsets_iter);
					reset_pin(*offsets_iter);
					release(*offsets_iter);
				}
			}
		}
		recycle(sbl_node->block_num);
        prt_node->key_num -= 1;
        write_back(prt_node->block_num);
        write_back(cur_node->block_num);
        reset_pin(prt_node->block_num);
        reset_pin(cur_node->block_num);
		if (prt_node->is_root)
		{
			if (prt_node->key_num == 0)
			{
				// copy cur_node to prt_node
				if (!cur_node->is_leaf)
				{
					vector<int>::iterator offsets_iter = cur_node->offsets.begin();
					for ( ; offsets_iter != cur_node->offsets.end(); offsets_iter++)
					{
						fetch(*offsets_iter);
						set_pin(*offsets_iter);
						nodes[*offsets_iter]->parent = prt_node->block_num;
						write_back(*offsets_iter);
						reset_pin(*offsets_iter);
						release(*offsets_iter);
					}
				}
				prt_node->parent = -1;
				prt_node->next = -1;
				prt_node->is_root = true;
				prt_node->is_leaf = cur_node->is_leaf;
				prt_node->key_num = cur_node->key_num;
				prt_node->keys.swap(cur_node->keys);
				prt_node->offsets.swap(cur_node->offsets);

				recycle(cur_node->block_num);
                write_back(prt_node->block_num);
                reset_pin(prt_node->block_num);
			}
		}
		else if (prt_node->key_num * 2 < max_key_num)
		{
			redistribute(prt_node->block_num);
		}
	}
}

void BPlusTree::recycle(int block_num)
{	
	if (nodes.find(block_num) != nodes.end())
	{
		if (nodes[block_num] != NULL)
			delete nodes[block_num];
		nodes.erase(block_num);
		reset_pin(block_num);
		bm->deleteIndexBlock(table, attr, block_num);
		blocks.erase(block_num);
	}
}

KeyValue BPlusTree::find_min(int block_num)
{
	fetch(block_num);

	if (nodes[block_num]->is_leaf)
		return nodes[block_num]->keys[0];
	else
		return find_min(nodes[block_num]->offsets[0]);
}

bool BPlusTree::build(vector<pair<KeyValue, int> > init)
{
	for (vector<pair<KeyValue, int> >::iterator iter = init.begin(); iter != init.end(); iter++)
	{
		insert(iter->first, iter->second);
	}
    /*
	if (init.size() <= max_key_num)
	{
		fetch(0);
		set_pin(0);
		for (vector<pair<KeyValue, int> >::iterator iter = init.begin(); iter != init.end(); iter++)
		{
			nodes[0]->keys.push_back(iter->first);
			nodes[0]->offsets.push_back(iter->second);
			nodes[0]->key_num += 1;
		}
		write_back(0); 
		reset_pin(0);
		release(0);
	}
	else
	{
		vector<pair<KeyValue, int> > children;
		int n = init.size();
		int prev = -1;
		for (int i = 0; i < n/max_key_num-1; i++)
		{
			IndexBlock *tmp = bm->getIndexNewBlock(table, attr);
			set_pin(tmp->blockNo);
			blocks[tmp->blockNo] = tmp;
			nodes[tmp->blockNo] = new Node(tmp->blockNo);
			Node *cur_node = nodes[tmp->blockNo];
			cur_node->prev = prev;
			cur_node->block_num = tmp->blockNo;
			cur_node->is_leaf = true;
			cur_node->is_root = false;
			cur_node->key_num = max_key_num;
			cur_node->next = -1;
			if (prev != -1)
				nodes[cur_node->prev]->next = cur_node->block_num;
			for (int j = 0; j < max_key_num; j++)
			{
				cur_node->keys.push_back(init.front().first);
				cur_node->offsets.push_back(init.front().second);
				init.erase(init.begin());
			}
			children.push_back(make_pair(cur_node->keys.front(), cur_node->block_num));
			// write_back(tmp->blockNo);
			// reset_pin(tmp->blockNo);
			// release(tmp->blockNo);
		}
		for (int n = init.size()/2; init.size() != 0; n = init.size())
		{
			IndexBlock *tmp = bm->getIndexNewBlock(table, attr);
			set_pin(tmp->blockNo);
			blocks[tmp->blockNo] = tmp;
			nodes[tmp->blockNo] = new Node(tmp->blockNo);
			Node *cur_node = nodes[tmp->blockNo];
			cur_node->prev = prev;
			cur_node->block_num = tmp->blockNo;
			cur_node->is_leaf = true;
			cur_node->is_root = false;
			cur_node->key_num = n;
			cur_node->next = -1;
			if (prev != -1)
				nodes[cur_node->prev]->next = cur_node->block_num;
			for (int j = 0; j < n; j++)
			{
				cur_node->keys.push_back(init.front().first);
				cur_node->offsets.push_back(init.front().second);
				init.erase(init.begin());
			}
			children.push_back(make_pair(cur_node->keys.front(), cur_node->block_num));
		}
		recur_build(children);
	}
     */
	return true;
}

bool BPlusTree::recur_build(vector<pair<KeyValue, int> > init)
{
	if (init.size() <= max_key_num+1)
	{
		fetch(0);
		set_pin(0);
		int i = 0;
		for (vector<pair<KeyValue, int> >::iterator iter = init.begin(); iter != init.end(); iter++)
		{
			if (iter != init.begin())
				nodes[0]->keys.push_back(iter->first);
			nodes[0]->offsets.push_back(iter->second);
			nodes[0]->key_num += 1;
			nodes[iter->second]->parent = 0;
			nodes[iter->second]->child_num = i;
			write_back(iter->second);
			reset_pin(iter->second);
			release(iter->second);
			i++;
		}
		write_back(0);
		reset_pin(0);
		release(0);
	}
	else
	{
		vector<pair<KeyValue, int> > children;
		int n = init.size();
		int prev = -1;
		for (int i = 0; i < n / (max_key_num+1) - 1; i++)
		{
			IndexBlock *tmp = bm->getIndexNewBlock(table, attr);
			set_pin(tmp->blockNo);
			blocks[tmp->blockNo] = tmp;
			nodes[tmp->blockNo] = new Node(tmp->blockNo);
			Node *cur_node = nodes[tmp->blockNo];
			cur_node->prev = prev;
			cur_node->block_num = tmp->blockNo;
			cur_node->is_leaf = true;
			cur_node->is_root = false;
			cur_node->key_num = max_key_num;
			cur_node->next = -1;
			if (prev != -1)
				nodes[cur_node->prev]->next = cur_node->block_num;
			int child_num = 0;
			for (int j = 0; j < (max_key_num+1); j++, child_num++)
			{
				if (j == 0)
					children.push_back(make_pair(cur_node->keys.front(), cur_node->block_num));
				else
					cur_node->keys.push_back(init.front().first);
				cur_node->offsets.push_back(init.front().second);
				nodes[init.front().second]->parent = cur_node->block_num;
				nodes[init.front().second]->child_num = child_num;
				write_back(init.front().second);
				reset_pin(init.front().second);
				release(init.front().second);
				init.erase(init.begin());
			}
		}
		for (int n = init.size() / 2; init.size() != 0; n = init.size())
		{
			IndexBlock *tmp = bm->getIndexNewBlock(table, attr);
			set_pin(tmp->blockNo);
			blocks[tmp->blockNo] = tmp;
			nodes[tmp->blockNo] = new Node(tmp->blockNo);
			Node *cur_node = nodes[tmp->blockNo];
			cur_node->prev = prev;
			cur_node->block_num = tmp->blockNo;
			cur_node->is_leaf = true;
			cur_node->is_root = false;
			cur_node->key_num = n-1;
			cur_node->next = -1;
			if (prev != -1)
				nodes[cur_node->prev]->next = cur_node->block_num;
			int child_num = 0;
			for (int j = 0; j < n; j++, child_num++)
			{
				if (j == 0)
					children.push_back(make_pair(cur_node->keys.front(), cur_node->block_num));
				else
					cur_node->keys.push_back(init.front().first);
				cur_node->offsets.push_back(init.front().second);
				nodes[init.front().second]->parent = cur_node->block_num;
				nodes[init.front().second]->child_num = child_num;
				write_back(init.front().second);
				reset_pin(init.front().second);
				release(init.front().second);
				init.erase(init.begin());
			}
			children.push_back(make_pair(cur_node->keys.front(), cur_node->block_num));
		}
		recur_build(children);
	}
	return true;
}

int BPlusTree::search(KeyValue key)
{
	return recur_search(0, key);
}

vector<int> BPlusTree::search(KeyValue lower, bool lowerIn, KeyValue upper, bool upperIn)
{
	vector<int> indices;
	if (lowerIn && upperIn && lower > upper)
		return indices;
	if (!lowerIn && upperIn && lower >= upper)
		return indices;
	if (lowerIn && !upperIn && lower >= upper)
		return indices;
	if (!lowerIn && !upperIn && lower >= upper)
		return indices;
	recur_search(0, indices, lower, lowerIn, upper, upperIn);
	return indices;
}

void BPlusTree::recur_search(int block_num, vector<int> &indices, KeyValue lower, bool lowerIn, KeyValue upper, bool upperIn)
{
	fetch(block_num);

	Node *cur_node = nodes[block_num];
	if (cur_node->is_leaf)
	{
		int i;
		for (i = 0; i < cur_node->key_num; i++)
		{
			if (lowerIn ? lower <= cur_node->keys[i] : lower < cur_node->keys[i])
				break;
		}
		while (upperIn ? cur_node->keys[i] <= upper : cur_node->keys[i] < upper)
		{
			indices.push_back(cur_node->offsets[i]);
            i++;
			if (i == cur_node->key_num && cur_node->next != -1)
			{
				fetch(cur_node->next);
				cur_node = nodes[cur_node->next];
				release(block_num);
				i = 0;
				block_num = cur_node->next;
			}
		}
		release(block_num);
		return;
	}
	else
	{
		int i;
		for (i = 0; i < cur_node->key_num; i++)
		{
			if (cur_node->keys[i] > lower)
			{
				break;
			}
		}
		recur_search(nodes[block_num]->offsets[i], indices, lower, lowerIn, upper, upperIn);
		release(block_num);
		return;
	}
}

void BPlusTree::recur_print(int block_num)
{
    fetch(block_num);
    for (int i = 0; !nodes[block_num]->is_leaf && i < nodes[block_num]->offsets.size(); i++)
        recur_print(nodes[block_num]->offsets[i]);
    map<int, Node *>::iterator map_iter = nodes.find(block_num);
    for (; map_iter != nodes.end(); map_iter = nodes.end())
    {
        if (map_iter->first != map_iter->second->block_num)
        {
            cout << "block_num mismatch in node" << map_iter->first << endl;
        }
        if (map_iter->second->key_num != map_iter->second->keys.size())
        {
            cout << "key_num mismatch in node" << map_iter->first << endl;
        }
        if (!map_iter->second->is_leaf && map_iter->second->parent != -1)
        {
            fetch(map_iter->second->parent);
            if (nodes[map_iter->second->parent]->offsets[map_iter->second->child_num] != map_iter->second->block_num)
            {
                cout << "parent or child_num mismatch in node" << map_iter->first << endl;
            }
        }
        if (map_iter->second->parent == -1 && !map_iter->second->is_root)
        {
            cout << "parent mismatch in node" << map_iter->first << endl;
        }
        if (map_iter->second->next != -1)
        {
            fetch(map_iter->second->next);
            if (nodes[map_iter->second->next]->prev != map_iter->second->block_num)
            {
                cout << "next mismatch in node" << map_iter->first << endl;
            }
        }
        if (map_iter->second->prev != -1)
        {
            fetch(map_iter->second->prev);
            if (nodes[map_iter->second->prev]->next != map_iter->second->block_num)
            {
                cout << "prev mismatch in node" << map_iter->first << endl;
            }
        }
        
        cout << "block_num: " << map_iter->first << endl;
        cout << "parent: " << map_iter->second->parent << endl;
        cout << "is_leaf: " << map_iter->second->is_leaf << endl;
        cout << "is_root: " << map_iter->second->is_root << endl;
        cout << "child_num: " << map_iter->second->child_num << endl;
        cout << "key_num: " << map_iter->second->key_num << endl;
        cout << "next: " << map_iter->second->next << endl;
        cout << "prev: " << map_iter->second->prev << endl;
        cout << "keys: " << endl;
        
        vector<KeyValue>::iterator keys_iter = map_iter->second->keys.begin();
        for (; keys_iter != map_iter->second->keys.end(); keys_iter++)
        {
            cout << *keys_iter << ' ';
        }
        cout << endl;
        
        cout << "offsets: " << endl;
        vector<int>::iterator offsets_iter = map_iter->second->offsets.begin();
        for (int i = 0; offsets_iter != map_iter->second->offsets.end(); offsets_iter++, i++)
        {
            if (!map_iter->second->is_leaf)
            {
                fetch(*offsets_iter);
                if (nodes[*offsets_iter]->child_num != i)
                {
                    cout << "child_num mismatch in node" << *offsets_iter << endl;
                }
            }
            cout << *offsets_iter << ' ';
        }
        cout << endl;
        
        cout << endl;
    }
    
}

// print tree for debugging
void BPlusTree::print()
{
    cout << "-----------------------START PRINTING-----------------------" << endl;
    recur_print(0);
    release_unpinned();
    cout << "------------------------------------------------------------" << endl;
}

// print tree for debugging
void BPlusTree::draw()
{
	vector<int> to_draw;
	to_draw.insert(to_draw.begin(), 0);
	cout << "-----------------------START PRINTING-----------------------" << endl;
	while (to_draw.size() != 0)
	{
		int cur = to_draw.front();
		to_draw.erase(to_draw.begin());
		switch (cur)
		{
		case -1: // element separator
			cout << " | ";
			break;
		case -2: // node separator
			cout << "    ";
			break;
		case -3: // newline
			cout << '\n' << '\n';
			if (to_draw.size() != 0)
				to_draw.push_back(-3);
			break;
		default:
			if (cur == 0)
				to_draw.push_back(-3);
			int i;
            fetch(cur);
			Node *n = nodes[cur];
			for (i = 0; i < n->keys.size(); i++)
			{
				cout << n->offsets[i];
				cout << " [" << n->keys[i] << "] ";
			}
			if (!n->is_leaf)
			{
				cout << n->offsets[i];
				for (i = 0; i < n->offsets.size(); i++)
				{
					if (i) to_draw.push_back(-1);
					to_draw.push_back(n->offsets[i]);
				}
				to_draw.push_back(-2);
			}
			
			break;
		}
	}
	cout << "------------------------------------------------------------" << endl;
}

int BPlusTree::recur_search(int block_num, KeyValue key)
{
	fetch(block_num);

	Node *cur_node = nodes[block_num];
	if (cur_node->is_leaf)
	{
		for (int i = 0; i < cur_node->key_num; i++)
		{
			if (cur_node->keys[i] == key)
				return cur_node->offsets[i];
		}
		return KEY_NOT_FOUND;
	}
	else
	{
		int i;
		for (i = 0; i < cur_node->key_num; i++)
		{
			if (cur_node->keys[i] > key)
			{
				int result = recur_search(nodes[block_num]->offsets[i], key);
				release(block_num);
				return result;
			}
		}
		int result = recur_search(cur_node->offsets[i], key);
		release(block_num);
		return result;
	}
}

void BPlusTree::destroy()
{
	recur_destroy(0);
}

void BPlusTree::recur_destroy(int block_num)
{
	fetch(block_num);
	set_pin(block_num);
	if (!nodes[block_num]->is_leaf)
	{
		vector<int>::iterator iter = nodes[block_num]->offsets.begin();
		for (; iter != nodes[block_num]->offsets.end(); iter++)
		{
			recur_destroy(*iter);
		}
	}
	else
	{
		recycle(block_num);
	}
}

Node::Node(char *p, int type)
{
	parent = *((int *)p);
	p += sizeof(int);
	next = *((int *)p);
	p += sizeof(int);
	prev = *((int *)p);
	p += sizeof(int);
	is_root = *((bool *)p);
	p += sizeof(bool);
	is_leaf = *((bool *)p);
	p += sizeof(bool);
	key_num = *((int *)p);
	p += sizeof(int);
	child_num = *((int *)p);
	p += sizeof(int);
	block_num = *((int *)p);
	p += sizeof(int);
	for (int i = 0; i < key_num; i++)
	{
		switch (type)
		{
		case -1:
			keys.push_back(*((int *)p));
			p += sizeof(int);
			break;
		case 0:
			keys.push_back(*((float *)p));
			p += sizeof(float);
			break;
		default:
            string s;
            s.append(p);
            p += type;
            keys.push_back(KeyValue(s, type));
			break;
		}
	}
	for (int i = is_leaf ? 0 : -1; i < key_num; i++)
	{
		offsets.push_back(*((int *)p));
		p += sizeof(int);
	}
}

Node::Node(int block_num)
{
	//TODO
    this->block_num = block_num;
	// DEBUG
	// block_num = i;
	// i++;
}