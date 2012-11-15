struct QueueElementTask
{
    struct TASK value;
    struct QueueElementTask *next;
};

struct QueueTask
{
    int count;
    struct QueueElementTask *first;
    struct QueueElementTask *last;
};

bool QueueIsEmptyTask(struct QueueTask *queue)
{
    return((queue->count) < 1);
}

void AddToQueueTask(struct QueueTask *queue, struct TASK value)
{
    struct QueueElementTask *element = malloc(sizeof(struct QueueElementTask));
    element->value = value;
    element->next = NULL;
    if(QueueIsEmptyTask(queue))
    {
        queue->first = element;
        queue->last = element;
    }
    else
    {
        queue->last->next = element;
        queue->last = element;
    }
    (queue->count)++;
}

struct TASK GetFromQueueTask(struct QueueTask *queue)
{
    if(!QueueIsEmptyTask(queue))
    {
        struct QueueElementTask *element;
        element = queue->first;
        queue->first = element->next;
        (queue->count)--;
        struct TASK value = element->value;
        free(element);
        return value;
    }
    else
    {
        printf("Queue is empty!\n");
        exit(-1);
    }
}
void InitializeQueueTask(struct QueueTask *queue)
{
    queue->count = 0;
    queue->first = NULL;
    queue->last = NULL;
}
