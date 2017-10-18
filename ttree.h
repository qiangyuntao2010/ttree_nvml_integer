//  
// Ttree.h: header file  
//  
// Copyright (C) QYT..  All rights reserved  
//  
// This source is free to use as you like.  If you make  
// any changes please keep me in the loop.  Email them to  
// qiangyuntao2010.  
//  
// PURPOSE:  
//  
//  To implement thread as a C object  
//  
// REVISIONS  
// =======================================================  
// Date: 2017.8.1   
// Name: qiangyuntao  
// Description: File creation  
//////////////////////////////////////////////////////////////////////  
#ifndef _TTREE_H_  
#define _TTREE_H_  

#include <assert.h>
#include <math.h>  
#include <assert.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <libpmem.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
#endif

#define KV_SIZE unsigned int
#define NODE_SIZE 2048
#define ITEM_NUM 125
#define MIN_KEY (125-2)
#define NODE_NUM (1024)
#define PATH "/home/qyt/ttree_nvml_integer/mydata"
#define PMEM_LEN ((off_t)(1<<30))



namespace stx{

typedef struct _pmem_node
{
    bool is_empty;
    //int  node_id;
    char *start_add;
}PMEM_NODE;


typedef struct _pm_meta_data
{
    PMEM_NODE *address[NODE_NUM];
}PM_META;

typedef struct _key_value_pair
{
    KV_SIZE key;
    KV_SIZE value;
}kv_t;

/*typedef struct meta_node
{
    unsigned long int  dm_node_num;
}DM_META;*/
  
typedef struct tagTTREENODE  
{   
    char *pmem_add;
    unsigned int pmem_id;
	tagTTREENODE *left;         // Left child pointer.  
    tagTTREENODE *right;        // Right child pointer.  
    int nItems;  // Internal node items.  
    kv_t item[ITEM_NUM];
    char bf;                   // Balabce factor(bf = right subtree height - left subtree height)     
}TTREENODE;  

template  <typename Key>
class ttree_default_set_traits
{
    static const bool selfverify = false;
    static const bool debug = false;
};

enum TraverseOrder  
{ 
    PreOrder,  
    InOrder,  
    PostOrder  
};

class CTtree{

private:

TTREENODE *root;

//DM_META *dm_meta;

PM_META *pm_meta;

char* pmemaddr;

int is_pmem;

public:

CTtree()
{
    std::cout<<"Constructor is called"<<std::endl;

    root = NULL;

    /*if((dm_meta = (DM_META*)malloc(sizeof(DM_META))) == NULL)
    {
        perror("d_meta malloc error!");
    }*/

    if((pm_meta = (PM_META*)malloc(sizeof(PM_META))) == NULL)
    {
        perror("pm_meta malloc error!");
    }

    if((pmemaddr = (char*)malloc(sizeof(char))) == NULL)
    {
        perror("pmemaddr malloc error!");
    }
}

public:
    
~CTtree()
{
    std::cout<<"Destructor is called!"<<std::endl;
}

public:

char *init_alloc()
{
    
    size_t mapped_len;
    int count;
    int fd;
    
    if ((pmemaddr = (char*)(pmem_map_file(PATH,PMEM_LEN,PMEM_FILE_CREATE,0666,&mapped_len,&is_pmem))) == NULL)
    {
        perror("pmem_map_file");
        exit(1);
    }
    char* log = pmemaddr + NODE_SIZE * 2;
    
    fprintf(stdout,"THE PMEM NODE SIZE IS %ld \n",sizeof(PMEM_NODE));
    fprintf(stdout,"THE DRAM NODE SIZE IS %ld \n",sizeof(TTREENODE));
    for(count = 0;count < NODE_NUM;count++)
    {
        pm_meta->address[count] = (PMEM_NODE*)malloc(sizeof(PMEM_NODE));
        if((pm_meta->address[count]->start_add = log + count * NODE_SIZE) == NULL)
        {
            perror("pmem allocation error!");
            exit(1);
        }
        //pm_meta->address[count]->node_id = count;
        pm_meta->address[count]->is_empty = true;
    }
    return log;
}

char *find_empty_node(TTREENODE *pNode)
{
    int count = 0;
    for(;count < NODE_NUM;count++)
    {
        if(pm_meta->address[count]->is_empty == true)
        {
            pNode->pmem_id = count;
      //      pNode->pmem_add = pm_meta->address[count]->start_add;
            pm_meta->address[count]->is_empty = false;
            return pm_meta->address[count]->start_add;
        }
             
    }
    if(count == NODE_NUM - 1)
    {
        return NULL;
    }
}

bool pmem_memcpy(char* paddr,TTREENODE* srcaddr,int cc)
{
    if(is_pmem)
    {
        pmem_memcpy_persist(paddr,srcaddr,cc);
        return true;
    }
    else
    {
        memcpy(paddr,srcaddr,cc);
        pmem_msync(paddr,cc);
        return true;
    }
    return false;
}

TTREENODE* FindMin(TTREENODE *pNode)  
{  
    if (pNode != NULL)  
    {  
        if (pNode->left == NULL)  
        { 
			fprintf(stdout,"Function %s:The node don not have left node\n",__func__);
            return pNode;  
        }  
        else  
        {  
            return FindMin(pNode->left);  
        }  
    }
    fprintf(stdout,"FUNCTION %s : THE NODE IS EMPTY",__func__);
    return NULL;  
}  

TTREENODE* FindMax(TTREENODE *pNode)  
{  
    if (pNode != NULL)  
    {  
        if (pNode->right == NULL)  
        {  
            return pNode; 
			fprintf(stdout,"Function %s:The node do not have right node\n",__func__);
        }  
        else  
        {  
            return FindMax(pNode->right);  
        }  
    }  
    fprintf(stdout,"FUNCTION %s : THE NODE IS EMPTY",__func__);
    return NULL;  
}  
  
  int count = 0;
  
unsigned long int Find(const unsigned long int key)  
{ 
    //int count;
    count++;
    printf("====count : %d\n",count);
    TTREENODE *pNode = root;  
    while (pNode != NULL)  
    {  
        int n = pNode->nItems;  
        unsigned long int keymin = pNode->item[0].key;  
        unsigned long int keymax = pNode->item[n > 0 ? n - 1 : 0].key;  
        int nDiff1 = keycompare(key, keymin);  
        int nDiff2 = keycompare(key, keymax);  
        if (nDiff1 >= 0 && nDiff2 <= 0)  
        {  
            int l = 0, r = n-1;  
            // Binary search.  
            while (l <= r)  
            {  
                int i = (l + r) >> 1;  
                unsigned long int itemkey = pNode->item[i].key;  
                int nDiff = keycompare(key, itemkey);  
                if (nDiff == 0)  
                {  
                    return pNode->item[i].value;  
                }  
                else if (nDiff > 0)  
                {   
                    l = i + 1;  
                }   
                else  
                {   
                    r = i - 1;                
                }  
            }  
            break;  
        }  
        else if (nDiff1 < 0)  
        {  
            pNode = pNode->left;  
        }  
        else if (nDiff2 > 0)  
        {  
            pNode = pNode->right;  
        }  
    }  
    fprintf(stdout,"The function %s : Can not find the k-v pair\n",__func__);
    return NULL;  
}  
  
int BalanceFactor(TTREENODE *pNode) const  
{  
    int l, r;     
    TTREENODE *p1, *p2;  
    l = r = 0;  
    p1 = p2 = pNode;  
    if (p1 != NULL)  
    {  
        while (p1->left != NULL)  
        {  
            p1 = p1->left;  
            l++;  
        }  
    }  
    if (p2 != NULL)  
    {  
        while (p2->right != NULL)  
        {  
            p2 = p2->right;  
            r++;  
        }  
    }  
    return (r - l);  
}  
  
int Depth()  
{  
    int l, r;     
    TTREENODE *p1, *p2;  
    l = r = 0;  
    p1 = p2 = root;  
    if (p1 != NULL)  
    {  
        while (p1->left != NULL)  
        {  
            p1 = p1->left;  
            l++;  
        }  
    }  
    if (p2 != NULL)  
    {  
        while (p2->right != NULL)  
        {  
            p2 = p2->right;  
            r++;  
        }  
    }  
    return Max(l, r);  
}  
  
const TTREENODE *GetMinNode()  //P
{  
    return FindMin(root);  
}  
  
const TTREENODE *GetMaxNode() //P  
{  
    return FindMax(root);  
}  
  
int Max( int a, int b ) const  
{  
    return (a > b ? a : b);  
}  
  
/** 
* Rotate T-tree node with left child.this is a single rotation for case LL. 
* Update balance factor, then return new root. 
*/  
TTREENODE *SingleRotateLeft(TTREENODE *pNode)  
{  
   TTREENODE *K = pNode->left;  
    pNode->left = K->right;  
    K->right = pNode;  
      
    // Adjust the balance factor.  
    pNode->bf = BalanceFactor(pNode);  
    K->bf = BalanceFactor(K);
    if((pmem_memcpy(pmemaddr,pNode,NODE_SIZE)&&(pmem_memcpy(pmemaddr+NODE_SIZE,K,NODE_SIZE))) == true)
    {
    pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
    pmem_memcpy(K->pmem_add,K,NODE_SIZE);
    }
    else
    {
        perror("log error!");
    }
    return K;  // new root  
}     
  
/** 
* Rotate T-tree node with right child.this is a single rotation for case RR. 
* Update balance factor, then return new root. 
*/  
TTREENODE *SingleRotateRight(TTREENODE *pNode)  
{  
    TTREENODE *K = pNode->right;  
    pNode->right = K->left;  
    K->left = pNode;  
      
    // Adjust the balance factor.  
    pNode->bf = BalanceFactor(pNode);  
    K->bf = BalanceFactor(K);  
    if((pmem_memcpy(pmemaddr,pNode,NODE_SIZE)&&(pmem_memcpy(pmemaddr+NODE_SIZE,K,NODE_SIZE))) == true)
    {
    pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
    pmem_memcpy(K->pmem_add,K,NODE_SIZE);
    }
    else
    {
        perror("log error!");
    }
    return K;  // new root  
}  
  
/** 
* Rotate T-tree node with left child.this is a double rotation for case LR. 
* Update balance factor, then return new root. 
*/   
TTREENODE *DoubleRotateLeft(TTREENODE *pNode)  
{  
    pNode->left = SingleRotateRight(pNode->left);  
      
    // Adjust the balance factor.  
    pNode->bf = BalanceFactor(pNode);  
    
    if((pmem_memcpy(pmemaddr,pNode,NODE_SIZE)) == true)
    {
    pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
    }
    else
    {
        perror("log error!");    
    }
    return SingleRotateLeft(pNode);  
}     
   
/** 
* Rotate T-tree node with right child.this is a double rotation for case RL. 
* Update balance factor, then return new root. 
*/   
TTREENODE *DoubleRotateRight(TTREENODE *pNode)  
{  
    pNode->right = SingleRotateLeft(pNode->right);  
      
    // Adjust the balance factor.  
    pNode->bf = BalanceFactor(pNode);  
    if((pmem_memcpy(pmemaddr,pNode,NODE_SIZE)) == true)
    {
    pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
    }
    else
    {
        perror("log error!");    
    }
    return SingleRotateRight(pNode);  
}     
  
void Insert(unsigned long int key, unsigned long int value)  
{  

    if (root == NULL)  
    {
        root = (TTREENODE*)malloc(NODE_SIZE);
        root->pmem_add = init_alloc(); 
        root->item[0].key = key;
        root->item[0].value =value;
        root->nItems = 1;  
        root->left = NULL;  
        root->right = NULL;
        root->bf = 0;
    }  
    else 
    {  
        TTREENODE *pNode = root; 
        bool bRet = _insert(pNode, key, value);
        if (pNode != root)  
        {   
            root = pNode;  
        }  
    }  
}       
  
void FreeNode(TTREENODE *pNode)
{
    if(pNode)
    {
       // dm_meta->dm_node_num--;
        pm_meta->address[pNode->pmem_id]->is_empty = true;
        pmem_msync(&pm_meta->address[pNode->pmem_id]->is_empty, 1); 
        //free(pNode);  
        pNode = NULL;
    }
    else 
    {
        fprintf(stdout,"THE FUNCTION %s : THE NODE IS EMPTY\n",__func__);
    }
}  
  
TTREENODE *MallocNode()  
{  
    TTREENODE *pNode;
    pNode = (TTREENODE*)(malloc(NODE_SIZE));  
    memset(pNode, 0x00, NODE_SIZE);  
  //  dm_meta->dm_node_num++;
    if((pNode->pmem_add = find_empty_node(pNode)) == NULL)
    {
       
        fprintf(stdout,"THE FUNCTION %s : NO MORE PMEM PLACE\n",__func__);
        exit(1);
    }
    return (pNode);  
}  
  
bool _insert(TTREENODE *pNode, unsigned long int key, unsigned long int value)  
{  
    int n = pNode->nItems;  
    unsigned long int keymin = pNode->item[0].key;  
    unsigned long int keymax = pNode->item[n > 0 ? n - 1 : 0].key;  
    int nDiff = keycompare(key, keymin);  
    if (nDiff <= 0)  
    {  
        TTREENODE *pLeftId = pNode->left;  
        if ((pLeftId == 0 || nDiff == 0 ) && pNode->nItems != ITEM_NUM)  
        {   
            for (int i = n; i > 0; i--)   
            {  
                pNode->item[i].key = pNode->item[i-1].key;
                pNode->item[i].value = pNode->item[i-1].value;
            }
            pNode->item[0].key = key;
            pNode->item[0].value = value;
            pNode->nItems += 1;
            if((pmem_memcpy(pmemaddr,pNode,NODE_SIZE)) == true)
            {
            pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
            }
            else
            {
            perror("log error!");    
            }
        }   
        if (pLeftId == 0)   
        {   
            pLeftId = MallocNode();  
            pNode->item[0].key = key;
            pNode->item[0].value = value;
            pLeftId->nItems += 1;          
            pNode->left = pLeftId; 
            if((pmem_memcpy(pmemaddr,pNode,NODE_SIZE))&&(pmem_memcpy(pmemaddr+NODE_SIZE,pLeftId,NODE_SIZE)) == true)
            {
            pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
            pmem_memcpy(pLeftId->pmem_add,pLeftId,NODE_SIZE);
            }
            else
            {
            perror("log error!");    
            }
        }  
        else   
        {  
            TTREENODE *pChildId = pLeftId;  
            bool bGrow = _insert(pChildId, key, value);  
            if (pChildId != pLeftId)  
            {   
                pNode->left = pLeftId = pChildId;  

            }  
            if (!bGrow)   
            {
                return false;  
            }  
        }  
        if (pNode->bf > 0)   
        {   
            pNode->bf = 0;
            if((pmem_memcpy(pmemaddr,pNode,NODE_SIZE)) == true)
            {
            pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
            }
            else
            {
            perror("log error!");    
            }
            return false;  
        }   
        else if (pNode->bf == 0)   
        {   
            pNode->bf = -1;     
            if((pmem_memcpy(pmemaddr,pNode,NODE_SIZE)) == true)
            {
            pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
            }
            else
            {
            perror("log error!");    
            }
            return true;  
        }   
        else   
        {   
            if (pLeftId->bf < 0)   
            {   
                pNode = SingleRotateLeft(pNode); // single LL turn  
            }   
            else   
            {   
                pNode = DoubleRotateLeft(pNode); // double LR turn    
            }         
            return false;  
        }  
          
    }          
    nDiff = keycompare(key, keymax);  
    if (nDiff >= 0)  
    {  
        TTREENODE *pRightId = pNode->right;  
        if ((pRightId == 0 || nDiff == 0 ) && pNode->nItems != ITEM_NUM)  
        {   
            pNode->item[n].key = key;
            pNode->item[n].value = value;
            pNode->nItems += 1;  
            if((pmem_memcpy(pmemaddr,pNode,NODE_SIZE)) == true)
            {
            pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
            }
            else
            {
            perror("log error!");    
            }
            return false;  
        }   
        if (pRightId == 0)   
        {   
            pRightId = MallocNode();  
            pRightId->item[0].key = key;
            pRightId->item[0].value = value;
            pRightId->nItems += 1;  
            pNode->right = pRightId;
            if(((pmem_memcpy(pmemaddr,pNode,NODE_SIZE)) && (pmem_memcpy(pmemaddr+NODE_SIZE,pRightId,NODE_SIZE)))== true)
            {
            pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
            pmem_memcpy(pRightId->pmem_add,pRightId,NODE_SIZE);
            }
            else
            {
            perror("log error!");    
            }
        }  
        else   
        {  
            TTREENODE *pChildId = pRightId;  
            bool bGrow = _insert(pChildId, key, value);  
            if (pChildId != pRightId)  
            {   //P
                pNode->right = pRightId = pChildId;  
            }  
            if (!bGrow)   
            {  
                return false;  
            }  
        }  
        if (pNode->bf < 0)   
        {   
            pNode->bf = 0;  
            if((pmem_memcpy(pmemaddr,pNode,NODE_SIZE)) == true)
            {
            pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
            }
            else
            {
            perror("log error!");    
            }
            return false;  
        }   
        else if (pNode->bf == 0)   
        {   
            pNode->bf = 1;  
            if((pmem_memcpy(pmemaddr,pNode,NODE_SIZE)) == true)
            {
            pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
            }
            else
            {
            perror("log error!");    
            }
            return true;  
        }   
        else   
        {   
            if (pRightId->bf > 0)   
            {   
                pNode = SingleRotateRight(pNode); // single RR turn  
            }   
            else   
            {   
                pNode = DoubleRotateRight(pNode); // double RL turn   
            }         
            return false;  
        }  
    }      
      
    int l = 1, r = n-1;  
    while (l < r)  
    {  
        int i = (l + r) >> 1;
        unsigned long int itemkey = pNode->item[i].key;
        nDiff = keycompare(key, itemkey);  
        if (nDiff > 0)  
        {   
            l = i + 1;  
        }   
        else  
        {   
            r = i;  
            if (nDiff == 0)  
            {   
                break;  
            }  
        }  
    }  
      
    // Insert before item[r]  
    if (n != ITEM_NUM)   
    {  
        for (int i = n; i > r; i--)   
        {  
            pNode->item[i].key = pNode->item[i-1].key;
        }  
        pNode->item[r].key = key;
        pNode->nItems += 1;  
        if((pmem_memcpy(pmemaddr,pNode,NODE_SIZE)) == true)
        {
        pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
        }
        else
        {
        perror("log error!");    
        }
        return false;  
    }  
    else   
    {   
        unsigned long int reinsertId;  
        unsigned long int reinsertData;  
        // The right than the left subtree subtree weight, into the left equilibrium.  
        if (pNode->bf >= 0)   
        {   
            // Node in the value of the most left out,   
            // key inserted into its position in the r-1.  
            // Value will be inserted into the left of its left subtree.  
            reinsertId= pNode->item[0].key;
            reinsertData = pNode->item[0].value;
            for (int i = 1; i < r; i++)  
            {  
                
                pNode->item[i-1].key = pNode->item[i].key;
                pNode->item[i-1].value = pNode->item[i].value;
            }  
                pNode->item[r-1].key = key;
                pNode->item[r-1].value = value;
              
            return _insert(pNode, reinsertId, reinsertData);          
        }   
        else // The left than the right subtree subtree re-insert the right balance.  
        {   
            // Node in the value of the most right out,   
            // key inserted into the location of its r.  
            // The right value will be inserted to its right subtree.  
            reinsertId = pNode->item[n-1].key;
            reinsertData = pNode->item[n-1].value;
            for (int i = n-1; i > r; i--)   
            {
                pNode->item[i].key = pNode->item[i-1].key;
                pNode->item[i].value = pNode->item[i-1].value;
            } 
                pNode->item[r].key = key;
                pNode->item[r].value = value;
              
            return _insert(pNode, reinsertId, reinsertData);  
        }     
    }  
}    
  
void Clear()   
{  
    _earse(root);     
}  
  
void _earse(TTREENODE *pNode)   
{  
    if (pNode == NULL)  
    {  
        return;  
    }  
  
    _earse(pNode->left);  
      
    _earse(pNode->right);  
  
    FreeNode(pNode);  
}  
int count1 = 0;

void Delete(KV_SIZE  key)  
{ 
    count1++;
    printf("===delete : %d and the key is %d===\n",count1,key);
    TTREENODE *pNode = root;  
    int h = remove(pNode, key);
    if(h < 0)
    {
        fprintf(stdout,"%s : miss!\n",__func__);
    }
    //assert(h >= 0);  
    if (pNode != root)  
    {   
        root = pNode;  
    }  
}  
  
int BalanceLeftBranch(TTREENODE *pNode)  
{  
    if (pNode->bf < 0)  
    {   
        pNode->bf = 0;
        pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
        return 1;  
    }   
    else if (pNode->bf == 0)  
    {   
       
        pNode->bf = 1;  
        pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
        return 0;  
    }   
    else  
    {   
        TTREENODE *pRightId = pNode->right;  
        if (pRightId->bf >= 0)   
        {   
            pNode = SingleRotateRight(pNode); // single RR turn  
            if (pRightId->bf == 0)  
            {  
                pNode->bf = 1;  
                pRightId->bf = -1;
                pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
                pmem_memcpy(pRightId->pmem_add,pRightId,NODE_SIZE);
                return 0;  
            }  
            else  
            {  
                pNode->bf = 0;  
                pRightId->bf = 0;  
                pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
                pmem_memcpy(pRightId->pmem_add,pRightId,NODE_SIZE);
                return 1;  
            }  
        }   
        else   
        {   
            pNode = DoubleRotateRight(pNode); // double RL turn   
            return 1;             
        }          
    }  
}  
  
int BalanceRightBranch(TTREENODE *pNode)  
{  
    if (pNode->bf > 0)  
    {   
        pNode->bf = 0;  
        pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
        return 1;  
    }   
    else if (pNode->bf == 0)  
    {   
        pNode->bf = -1;  
        pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
        return 0;  
    }   
    else  
    {   
        TTREENODE * pLeftId = pNode->left;  
        if (pLeftId->bf <= 0)   
        {   
            pNode = SingleRotateLeft(pNode); // single LL turn  
            if (pLeftId->bf == 0)  
            {  
                pNode->bf = -1;  
                pLeftId->bf = 1;  
                pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
                pmem_memcpy(pLeftId->pmem_add,pLeftId,NODE_SIZE);
                return 0;  
            }  
            else  
            {  
                pNode->bf = 0;  
                pLeftId->bf = 0;  
                pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
                pmem_memcpy(pLeftId->pmem_add,pLeftId,NODE_SIZE);
                return 1;  
            }  
        }   
        else   
        {   
            pNode = DoubleRotateLeft(pNode); // double LR turn    
            return 1;             
        }        
    }  
    return 0;  
}  
  
int remove(TTREENODE *pNode, KV_SIZE key)  
{  
    int n = pNode->nItems;  
    KV_SIZE keymin = pNode->item[0].key;  
    KV_SIZE keymax = pNode->item[n > 0 ? n - 1 : 0].key;  
    int nDiff = keycompare(key, keymin);  
    if (nDiff <= 0)  
    {   
        TTREENODE *pLeftId = pNode->left;  
        if (pLeftId != 0)  
        {   
            TTREENODE *pChildId = pLeftId;  
            int h = remove(pChildId, key);  
            if (pChildId != pLeftId)  
            {   
                pNode->left = pChildId;  
            }  
            if (h > 0)  
            {   
                int ret = BalanceLeftBranch(pNode);
                return ret;
            }  
            else if (h == 0)  
            {   
                //pmem_memcpy(pNode->pmem_add,pNode->left,NODE_SIZE);
                return 0;  
            }  
        }  
       // assert (nDiff == 0);  
    }  
    nDiff = keycompare(key, keymax);  
    if (nDiff <= 0)   
    {          
        for (int i = 0; i < n; i++)   
        {   
            if (pNode->item[i].key == key)  
            {   
                if (n == 1)   
                {   
                    if (pNode->right == 0)   
                    {   
                        TTREENODE *pTempNode = pNode->left;  
                        FreeNode(pNode);  
                        pNode = pTempNode;
                        //pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
                        return 1;  
                    }  
                    else if (pNode->left == 0)   
                    {   
                        TTREENODE *pTempNode = pNode->right;  
                        FreeNode(pNode);  
                        pNode = pTempNode;
                        //pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
                        return 1;  
                    }   
                }   
                TTREENODE *pLeftId = pNode->left, *pRightId = pNode->right;  
                if (n <= MIN_KEY)  
                {   
                    if (pLeftId != 0 && pNode->bf <= 0)  
                    {    
                        while (pLeftId->right != 0)   
                        {   
                            pLeftId = pLeftId->right;  
                        }  
                        while (--i >= 0)   
                        {   
                            pNode->item[i+1].key = pNode->item[i].key;
                            pNode->item[i+1].value = pNode->item[i].value;
                        }
                        pNode->item[0].key = pLeftId->item[pLeftId->nItems-1].key;
                        pNode->item[0].value = pLeftId->item[pLeftId->nItems-1].value;
                        key = pNode->item[0].key;  
                        TTREENODE *pChildId = pLeftId;  
                        int h = remove(pChildId, pNode->item[0].key);  
                        if (pChildId != pLeftId)   
                        {   
                            pNode->left = pChildId;  
                        }  
                        if (h > 0)   
                        {  
                            h = BalanceLeftBranch(pNode);  
                        }  
                        //pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
                        return h;  
                    }   
                    else if (pNode->right != 0)   
                    {   
                        while (pRightId->left != 0)  
                        {   
                            pRightId = pRightId->left;  
                        }  
                        while (++i < n)  
                        {   
                            pNode->item[i].key = pNode->item[i-1].key;
                            pNode->item[i].value = pNode->item[i-1].value;
                        }
                        pNode->item[n-1].key = pRightId->item[0].key;
                        pNode->item[n-1].value = pRightId->item[0].value;
                        key = pNode->item[n-1].key;
                        TTREENODE *pChildId = pRightId;  
                        int h = remove(pChildId, key);  
                        if (pChildId != pRightId)  
                        {   
                            pNode->right = pChildId;  
                        }  
                        if (h > 0)  
                        {  
                            h = BalanceRightBranch(pNode);  
                        }  
                        return h;  
                    }  
                }  
                while (++i < n)  
                {   
                    pNode->item[i-1].key = pNode->item[i].key;
                    pNode->item[i-1].value = pNode->item[i].value;
                }  
                pNode->nItems -= 1;  
                pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
                return 0;  
            }  
        }  
    }  
    TTREENODE *pRightId = pNode->right;  
    if (pRightId != 0)  
    {   
        TTREENODE *pChildId = pRightId;  
        int h = remove(pChildId, key);  
        if (pChildId != pRightId)  
        {  
            pNode->right = pChildId;  
        }  
        if (h > 0)  
        {   
            return BalanceRightBranch(pNode);  
        }  
        else  
        {   
            //pmem_memcpy(pNode->pmem_add,pNode,NODE_SIZE);
            return h;  
        }  
    }  
    return -1;  
}  
  
bool IsEmpty( ) const  
{  
    return root == NULL;  
}  
  
int keycompare(unsigned long int key1, unsigned long int key2)  
{
    unsigned long int p1 = key1;
    unsigned long int p2 = key2;
    return p1 < p2 ? -1 : p1 == p2 ? 0 : 1;
	
}  
  
void TraverseTree(TraverseOrder order)  
{  
    switch (order)  
    {  
    case PreOrder:  
        PreOrderTraverse(root);  
        break;  
    case InOrder:  
        InOrderTraverse(root);  
        break;  
    case PostOrder:  
        PostOrderTraverse(root);  
        break;  
   /* case LevelOrder:  
        LevelOrderTraverse(root);  
        break;*/  
    }  
}  
  
void InOrderTraverse(TTREENODE *pNode) const  
{   
    if (pNode != NULL)  
    {   
        InOrderTraverse(pNode->left);   
        int nSize = pNode->nItems;  
        for (int i = 0; i < nSize; i++)  
        {  
            printf("%ld ", pNode->item[i].key);  
        }   
        InOrderTraverse(pNode->right);   
    }  
}   
  
void PostOrderTraverse(TTREENODE *pNode) const  
{   
    if (pNode != NULL)  
    {   
        PostOrderTraverse(pNode->left);   
        PostOrderTraverse(pNode->right);   
        int nSize = pNode->nItems;  
        for (int i = 0; i < nSize; i++)  
        {  
            printf("%ld ", pNode->item[i].key);  
        }  
    }  
}   
    
void PreOrderTraverse(TTREENODE *pNode) const  
{   
    if (pNode != NULL)  
    {   
        int nSize = pNode->nItems;  
        for (int i = 0; i < nSize; i++)  
        {  
            printf("%ld ", pNode->item[i].key);  
        }  
        PreOrderTraverse(pNode->left);   
        PreOrderTraverse(pNode->right);   
    }    
}



};
}
  
#endif   
