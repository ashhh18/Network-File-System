#include "HashTable.h"
HT initTable()
{
    HT h = (HT)malloc(sizeof(hashTable));
    for (int i = 0; i < capacity; i++)
    {
        h->arr[i] = (Node)malloc(sizeof(node));
        h->arr[i]->next = NULL;
    }
    return h;
}

int hashingFunc(char *s)
{
    long long bigNumber = 1e17;
    int n = strlen(s);
    long long hash = 1;
    for (int i = 0; i < n; i++)
    {
        hash = (hash * PRIME) % bigNumber;
        hash = (hash + s[i]) % bigNumber;
    }
    return (int)(hash % capacity);
}

void insertNode(Node N, int e, char *s)
{
    while (N->next != NULL)
        N = N->next;
    N->next = (Node)malloc(sizeof(node));
    N->next->next = NULL;
    N->next->val = e;
    N->next->s = strdup(s);
}

void insert(int e, char *s, HT h)
{
    int hh = hashingFunc(s);
    insertNode(h->arr[hh], e, s);
}

int deletePath(char *s, HT h)
{
    // printf("--%s--\n",s);
    int idx = hashingFunc(s);
    Node toRemove = h->arr[idx];
    while (toRemove->next != NULL && strcmp(toRemove->next->s, s) != 0)
        toRemove = toRemove->next;
    if (toRemove->next == NULL)
    {
        return -1;
    }
    free(toRemove->next->s);
    Node toAssign = toRemove->next->next;
    free(toRemove->next);
    toRemove->next = toAssign;
    return 0;
}

int searchPath(char *s, HT h)
{
    int hashVal = hashingFunc(s);
    Node N = h->arr[hashVal]->next;
    int ret = -1;
    while (N != NULL)
    {
        if (strcmp(N->s, s) == 0)
        {
            ret = N->val;
            break;
        }
        N = N->next;
    }
    return ret;
}