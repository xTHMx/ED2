// C program
// Implement B + Tree
#include <stdio.h>
#include <stdlib.h>

// Define tree node
struct TreeNode
{
    int isLeaf;
    int *keys;
    int count;
    struct TreeNode **child;
};

// Define tree structure
struct BPTree
{
    struct TreeNode *root;
    int degree;
};



// Returns the new B+ tree
struct BPTree *createTree(int degree)
{

    // Allocate memory of new tree
    struct BPTree *tree = (struct BPTree *) malloc(sizeof(struct BPTree));

    if (tree != NULL)
    {
        // Set degree and root value
        tree->degree = degree;
        tree->root = NULL;
    }
    else
    {
        printf("\n Memory Overflow when create new Tree");
    }
}
// Returns new B+ tree node
struct TreeNode *createNode(int degree)
{
    // Create new tree node
    struct TreeNode *node = (struct TreeNode *) malloc(sizeof(struct TreeNode));
    
    if (node != NULL)
    {
        // Create memory of node key
        node->keys = (int *) malloc((sizeof(int)) *(degree));
        // Allocate memory of node child
        node->child = (struct TreeNode **) malloc((degree + 1) *sizeof(struct TreeNode *));
        // Set initial child
        for (int i = 0; i <= degree ; ++i)
        {
            node->child[i] = NULL;
        }
        node->isLeaf = 0;
        node->count = 0;
    }
}

struct TreeNode *findParent(struct TreeNode *cursor, struct TreeNode *child)
{
    struct TreeNode *parent = NULL;

    if (cursor->isLeaf == 1 || (cursor->child[0])->isLeaf == 1)
    {
        return NULL;
    }
    for (int i = 0; i < cursor->count + 1; i++)
    {
       
        if (cursor->child[i] == child)
        {
            parent = cursor;
            return parent;
        }
        else
        {
        
            parent = findParent(cursor->child[i], child);

            if (parent != NULL)
            {
                return parent;
            }
        }
    }
  
    return parent;
}

void insertInternal(struct BPTree *tree, int x, struct TreeNode *cursor, struct TreeNode *child)
{
      
    int i = 0;
    int j = 0;
    if (cursor->count < tree->degree) //has space
    {
     
        while (x > cursor->keys[i] && i < cursor->count)
        {
            i++;
        }
     
        for (j = cursor->count; j > i; j--)
        {
            cursor->keys[j] = cursor->keys[j - 1];
        }
 
        for (j = cursor->count + 1; j > i + 1; j--)
        {
            cursor->child[j] = cursor->child[j - 1];
        }
        cursor->keys[i] = x;
        cursor->count++;
        cursor->child[i + 1] = child;
    }
    else
    {


        struct TreeNode *newInternal = createNode(tree->degree);
        int virtualKey[tree->degree + 1];
        struct TreeNode *virtualPtr[tree->degree + 2];
      
        for (i = 0; i < tree->degree; i++)
        {
            virtualKey[i] = cursor->keys[i];
        }
        
        for (i = 0; i < tree->degree + 1; i++)
        {
            virtualPtr[i] = cursor->child[i];
        }
 
        i = 0;

        while (x > virtualKey[i] && i < tree->degree)
        {
            i++;
        }
        
        for (j = tree->degree + 1; j > i; j--)
        {
            virtualKey[j] = virtualKey[j - 1];
        }
        virtualKey[i] = x;
    
        for ( j = tree->degree + 2; j > i + 1; j--)
        {
            virtualPtr[j] = virtualPtr[j - 1];
        }
        virtualPtr[i + 1] = child;
        newInternal->isLeaf = 0;
        cursor->count = (tree->degree + 1) / 2;
        newInternal->count = tree->degree - (tree->degree + 1) / 2;
        for (i = 0, j = cursor->count + 1; i < newInternal->count; i++, j++)
        {
            newInternal->keys[i] = virtualKey[j];
        }
        for (i = 0, j = cursor->count + 1; i < newInternal->count + 1; i++, j++)
        {
            newInternal->child[i] = virtualPtr[j];
        }
     
        if (cursor == tree->root)
        {
           
            tree->root = createNode(tree->degree);

            tree->root->keys[0] = cursor->keys[cursor->count];

            tree->root->child[0] = cursor;

            tree->root->child[1] = newInternal;

            tree->root->isLeaf = 0;

            tree->root->count = 1;

        }
        else
        {
            insertInternal(tree, cursor->keys[cursor->count], findParent(tree->root, cursor), newInternal);
        }
    }
}
// Handles the request to add new node in B+ tree
void insert(struct BPTree *tree, int x)
{
    if (tree->root == NULL)
    {
        // Add first node of tree
        tree->root = createNode(tree->degree);
        tree->root->isLeaf  = 1;
        tree->root->count   = 1;
        tree->root->keys[0] = x;

    }
    else
    {
        // Loop controlling variables
        int i = 0;
        int j = 0;

        struct TreeNode *cursor = tree->root;

        struct TreeNode *parent = NULL;

        // Executes the loop until when cursor node is not leaf node
        while (cursor->isLeaf == 0)
        {
            // Get the current node
            parent = cursor;

            for (i = 0; i < cursor->count; i++)
            {
                if (x < cursor->keys[i])
                {
                    cursor = cursor->child[i];
                    break;
                }
                if (i == cursor->count - 1)
                {
                    cursor = cursor->child[i + 1];
                    break;
                }
            }
        }
        if (cursor->count < tree->degree)
        {
            i = 0;
            while (x > cursor->keys[i] && i < cursor->count)
            {
                i++;
            }
            for (j = cursor->count; j > i; j--)
            {
                cursor->keys[j] = cursor->keys[j - 1];
            }
            cursor->keys[i] = x;
            cursor->count++;
            cursor->child[cursor->count] = cursor->child[cursor->count - 1];
            cursor->child[cursor->count - 1] = NULL;
        }
        else
        {
    
            struct TreeNode *newLeaf = createNode(tree->degree);

            int virtualNode[tree->degree + 2];
 
            for (i = 0; i < tree->degree; i++)
            {
                virtualNode[i] = cursor->keys[i];
            }
            i = 0;
            while (x > virtualNode[i] && i < tree->degree)
            {
                i++;
            }
 
            for (j = tree->degree + 1; j > i; j--)
            {
                virtualNode[j] = virtualNode[j - 1];
            }
            virtualNode[i] = x;
            newLeaf->isLeaf = 1;
            cursor->count = (tree->degree + 1) / 2;
            newLeaf->count = tree->degree + 1 - (tree->degree + 1) / 2;
            cursor->child[cursor->count] = newLeaf;
            newLeaf->child[newLeaf->count] = cursor->child[tree->degree];
            cursor->child[tree->degree] = NULL;

            for (i = 0; i < cursor->count; i++)
            {
                cursor->keys[i] = virtualNode[i];
            }
            for (i = 0, j = cursor->count; i < newLeaf->count; i++, j++)
            {
                newLeaf->keys[i] = virtualNode[j];
            }
            if (cursor == tree->root)
            {
                struct TreeNode *newRoot = createNode(tree->degree);
             
                newRoot->keys[0] = newLeaf->keys[0];
                newRoot->child[0] = cursor;
                newRoot->child[1] = newLeaf;
                newRoot->isLeaf = 0;
                newRoot->count = 1;
                tree->root = newRoot;
            }
            else
            {
                insertInternal(tree, newLeaf->keys[0], parent, newLeaf);
            }
        }
    }
}

// Print the elements of B+ tree elements
void printNode(struct TreeNode *node) 
{
    if(node == NULL)
    {
        // When node is empty
        return;
    }
    else
    {
        int i  = 0;

        // iterate the node element
        while(i < node->count)
        {
            if(node->isLeaf==0)
            {
                // When node is not leaf
                printNode(node->child[i]);
            }
            else
            {
                // Print the left node value
                printf("  %d",node->keys[i]); 
            }
            i++;
        }
        if(node->isLeaf == 0)
        {
            printNode(node->child[i]);
        }
    }
}

int main(int argc, char const *argv[])
{
    // Create a new b+ tree with degree 4
    struct BPTree *tree = createTree(4);

    // Add node
    insert(tree,10);
    insert(tree,7);
    insert(tree,15);
    insert(tree,2);
    insert(tree,-3);
    insert(tree,12);
    insert(tree,35);
    insert(tree,14);

    // Print Tree elements
    printNode(tree->root);

    return 0;
}