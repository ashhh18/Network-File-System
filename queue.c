#include "defs.h"

void init(Queue q, int maxx)
{
    q->front = NULL;
    q->rear = NULL;
    q->size = 0;
    q->maxSize = maxx;
}

int isEmpty(Queue q)
{
    if (q->front == NULL)
        return 1;
    return 0;
}

int isFull(Queue q)
{
    if (q->size == q->maxSize)
        return 1;
    return 0;
}

int enqueue(Queue q, QNode E)
{
    if (!isFull(q))
    {
        QNode newNode = E;
        newNode->next = NULL;

        if (isEmpty(q))
        {
            q->front = newNode;
            q->rear = q->front;
        }
        else
        {
            q->rear->next = newNode;
            q->rear = newNode;
        }
        q->size++;
        return 0;
    }
    else
    {
        QNode temp = dequeue(q);
        enqueue(q, E);
        return 0;
    }
}

QNode removeMiddle(Queue q, int idx)
{
    if (!isEmpty(q))
    {
        if (q->size == 1)
            q->front = NULL;
        q->size--;
        int loc = 1; // 1 indexed
        QNode ptr = q->front;
        QNode prev = ptr;
        while (1)
        {
            if (loc < idx)
            {
                prev = ptr;
                ptr = ptr->next;
                loc++;
            }
            else
                break;
        }
        QNode temp = ptr;
        prev->next = ptr->next;
        return ptr;
    }
    return NULL;
}

QNode dequeue(Queue q)
{
    if (!isEmpty(q))
    {
        QNode val = q->front;
        q->front = q->front->next;

        if (q->front == NULL)
            q->rear = q->front;

        q->size--;
        return val;
    }
    return NULL;
}

int LRU(char *path, Queue q)
{
    int ans = 0;
    int flag = 0;
    int cur_size = 0;
    QNode curr = q->front;
    // printf("hekkekek\n");
    while (1)
    {
        if (curr != NULL)
        {
            curr = curr->next;
            cur_size++;
        }
        else
            break;
    }
    // printf("dsdfjhsdjfhjsdfj\n");
    curr = q->front;
    int idx = 0;
    for (int i = 1; i <= cur_size; i++)
    {
        if (strcmp(curr->s->cached_path, path) == 0)
        {
            ans = curr->s->quidx;
            flag = 1;
            idx = i;
            break;
        }
        else
        {
            curr = curr->next;
        }
    }
    if (flag == 1 && cur_size > 1)
    {
        QNode val = removeMiddle(q, idx);
        enqueue(q, val);
        return ans;
    }
    else if (flag != 1)
    {
        return -1;
    }
    return ans;
}
