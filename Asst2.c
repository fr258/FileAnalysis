#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>	
#include <pthread.h>
#include <fcntl.h>


#define add(X,Y) nodeAdd(&X, &Y, sizeof(Y))
#define nData(X) ((NameData*)X->data)
#define BUFFSIZE 256

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34;1m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_WHITE   "\x1b[37m"
#define ANSI_COLOR_RESET   "\x1b[0m"
 
typedef struct listNode
{
    void* data;
    struct listNode *next;
	
} Node;

typedef struct 
{
	char* name;
	double freq;
	
} NameData;

void initGeneral(Node* node, void* data, int dataSize);
void initNameData(Node* node, void* data, int temp);

typedef struct outputNodeList
{
    char* name1;
    char* name2;
    double distance;
    int count;
} outputNode;

typedef struct threadListNode
{
	pthread_t* data;
	struct threadListNode* next;
} threadNode;

typedef struct
{
	Node* head;
	
} Iterator;

typedef struct Arguments
{
  char* pathName;
  pthread_mutex_t* mut;
  Node* listHead;
} Args;

Node* next(Iterator* iter)
{
	if(iter->head != NULL)
	{
		Node* temp = iter->head;
		iter->head = iter->head->next;
		return temp; 
	}
	return NULL;
}

void* nextData(Iterator* iter)
{
	if(iter->head != NULL)
	{
		Node* temp = iter->head;
		iter->head = iter->head->next;
		return temp->data; 
	}
	return NULL;
}

int hasNext(Iterator* iter)
{
	return (iter->head != NULL);
}

//precondition: head isn't null and was initialized to {NULL, NULL}
Node* nodeAdd(Node* head, void* data, int dataSize)
{
	//printf("nodeAdd called\n");
	if(head->data != NULL)
	{
		while(head->next != NULL)
		{
			head = head->next;
		}
		
		head->next = malloc(sizeof(Node));
		head = head->next;
		
	}
	//printf("sequence 1 complete\n");
	if(dataSize == 3)
	{
		//printf("nodeAdd to call initNameData");
		initNameData(head, data, 0);
	}
	else
	{

		initGeneral(head, data, dataSize);
	}
	return head;
}

int compareStr(void* one, void* two)
{
	return strcmp(((NameData*)one)->name, ((NameData*)two)->name);
}

int compareCountNameData(void* one, void* two)
{
	return 0;
}

int compareCountOutput(void* one, void* two)
{
	return ((outputNode*)one)->count - ((outputNode*)two)->count;
}


Node* nodeAddSort(Node* head, void* data, int key)
{
	Node* front = head; 
	int (*compare) (void*, void*);
	void (*init) (Node*, void*, int);
	int size = 0;
	
	if(key == 0)
	{
		compare = compareCountNameData;
		init = initNameData;
	}
	else if(key == 1)
	{
		compare = compareStr;
		init = initNameData;
	}
	else if(key == 2)
	{
		compare = compareCountOutput;
		init = initGeneral;
		size = sizeof(outputNode);
	}

	if(nData(head)!=NULL)
	{
		Node *temp;
		int diff = compare(data, head->data);
		if(diff<0)
		{
			front = malloc(sizeof(Node));
			init(front, data, size);
			front->next = head;
		}
		else if(diff == 0)
		{
			nData(head)->freq++;
		}
		else
		{
			while(head->next != NULL && compare(data, head->next->data) >= 0)
			{
				head = head->next;
			}
			
			diff = compare(data, head->data);
			
			if(diff == 0 && key==1)
			{
				nData(head)->freq++;
			}
			else
			{
				temp = malloc(sizeof(Node));
				init(temp, data, size);
				
				temp->next = head->next;
				head->next = temp;	
			}

		}
	}
	else
	{
		init(head, data, size);
	}
	return front;
}

void initGeneral(Node* node, void* data, int dataSize)
{
    node->data = malloc(dataSize);
    memcpy(node->data, data, dataSize);
    node->next = NULL;
}

void initNameData(Node* node, void* in, int placeholder)
{
	NameData* data = (NameData*) in;
	node->data = malloc(sizeof(NameData));
	NameData* temp = nData(node);
	temp->name = strdup(data->name);
	temp->freq = data->freq; 
	node->next = NULL;
}

void deleteList(Node* head, size_t dataSize)
{
	Iterator iter = {head}, iter1 = {head};
	if(dataSize == 3)
	{
		NameData* temp;
		while(hasNext(&iter))
		{
			temp = nData(next(&iter));
			free(temp->name);
			free(temp);
		}
	}
	else
	{
		while(hasNext(&iter))
		{
			free(nextData(&iter));
		}
	}
	next(&iter1);
	while(hasNext(&iter1))
	{
		free(next(&iter1));
	}
}

void addToken(Node* node, NameData* data) 
{
	nData(node)->freq++;
	if(node->next == NULL)
	{
		node->next = malloc(sizeof(Node));
		node->next->next = NULL;
		node->next->data = NULL;
	}
	node->next = nodeAddSort(node->next, data, 1);
}

void tokenHelper(Node* node, char* name, int length)
{
	char temp[length+1]; //+1 for null terminator
	int i = 0, b = 0;
	for(i = 0, b = 0; i<length; i++)
	{
		if(isalpha(*(name+i)))
		{
			temp[b] = tolower(*(name+i));
			b++;
		}
		else if(*(name+i) == '-')
		{
			temp[b] = *(name+i);
			b++;
		}
	}
	temp[b] = '\0';
	if(b>0) //token contains at least 1 valid character
	{
		NameData data = {temp, 1};
		addToken(node, &data);
	}

}

void tokenHelperTwo(Node* node, char* token, char* buffer, char** temp1, int length, int* isCont)
{
	if(token-buffer+length < BUFFSIZE)
	{
		tokenHelper(node,token, strlen(token));
		*isCont = 0;
	}
	else
	{
		*temp1 = strdup(token);
		*isCont = 1;
	}
}

 char* my_strtok(char** currStr)
{
	int length = strlen(*currStr);
	int found = 0;
	int i = 0;
	
	for(i = 0; i<length && !found; i++)
	{		
		if((*currStr)[i] != ' ' && (*currStr)[i] != '\n')
		{
			found = 1;
			*(currStr) = *(currStr) + i;
		}
	}	
	if(found != 0)
	{
		found = 0;
		for(i = 0; i<length && !found; i++)
		{
			if((*currStr)[i]==' ' || (*currStr)[i]=='\n')
			{
				(*currStr)[i]='\0'; 
				found = 1;		
			}
		}	
		char* temp = *currStr;
		*currStr = *(currStr)+i;
		return temp;
	}
	return NULL;
}

void tokenizer(Node* node, int fd)
{
	int bytes;
	int length;
	char* buffer = malloc(BUFFSIZE+1);
	char* head = buffer; 
	int inBuffer;
	char* temp1 = NULL, *temp2 = NULL;
	int isCont = 0; //is continued from previous token?
	buffer[BUFFSIZE] = '\0';
	char* token;
	
	bytes =  read(fd, head, BUFFSIZE);
	
	token = my_strtok(&buffer); 

	while(bytes>0)
	{
		inBuffer = 0;
		while(token != NULL)
		{
			inBuffer = 1;
			length = strlen(token);
			
			if(isCont == 1)
			{			
				if(head[0]==' ' || head[0]=='\n')
				{
					tokenHelper(node, temp1, strlen(temp1));
					free(temp1);
					tokenHelperTwo(node, token, head, &temp1, length, &isCont);
				}
				else
				{
					temp2 = malloc(length + strlen(temp1)+1);
					strcpy(temp2, temp1);
					strcat(temp2, token);
					free(temp1);
					temp1 = temp2;
					if(token-head+length < BUFFSIZE)
					{
						tokenHelper(node, temp1, strlen(temp1));
						free(temp1);
						isCont = 0;
					}
				}
			}
			else
			{			
				tokenHelperTwo(node,token,head,&temp1,length,&isCont);				
			}	
		
			token = my_strtok(&buffer);

			
		} //end while token
		
		bytes =  read(fd, head, BUFFSIZE);
		buffer = head;
		buffer[bytes] = '\0';

		token = my_strtok(&buffer); 

		if(inBuffer == 0 && isCont == 1)
		{
			tokenHelper(node, temp1, strlen(temp1));
			free(temp1);
			isCont = 0;
			
		}
		
	} //end while bytes
	
	if(isCont==1 && temp1!=NULL)
	{
		tokenHelper(node, temp1, strlen(temp1));
		free(temp1);
	}
	free(head);
}


void* fileHandler(void* input)
{
	Args args = *(Args*)input;
	int fd = open(args.pathName, O_RDONLY);

	if(fd<0)
	{
		perror(args.pathName);
	}	
	else
	{
		Node* temp = malloc(sizeof(Node));
		temp->next = NULL;
		temp->data = NULL;	
		
		pthread_mutex_lock(args.mut);
		Node* temp2 = (Node*)(nodeAdd(args.listHead, temp, sizeof(*temp))->data);			
		pthread_mutex_unlock(args.mut);

		NameData data = {args.pathName, 0};		
		Node* tokenHead = nodeAdd(temp2, &data, 3);


		tokenizer(tokenHead, fd);

		int count = (int)nData(tokenHead)->freq;
		Iterator iter = {tokenHead};
		double* freq;
		next(&iter);
		while(hasNext(&iter))
		{
			freq = &nData(next(&iter))->freq;
			*freq = *freq / count;
		}
		free(temp);

	}
	close(fd);
	return NULL;
}

double findTokenName(Node* head, char* name)
{
	Node* temp = head;
	while(temp!=NULL)
	{
		if(strcmp(name, nData(temp)->name)==0)
		{
			//printf("found: %s", name);
			return nData(temp)->freq;
		}
		temp = temp->next;
	}
	return 0;
}


char* pathGenerator(char* path, char* name)
{
    int p = strlen(path);
    int n = strlen(name);
    char* newPath = malloc(p+n+2);
    for(int i = 0; i<p; i++)
    {
        newPath[i] = path[i];
    }
    newPath[p] = '/';
    for(int j = p+1; j<(n+p+1); j++)
    {
        newPath[j] = name[j-(p+1)];
    }
	
    return newPath;
}

void* directoryHandler(void* in)
{
	
	//printf("in directory handler!\n");
    //cast input to struct Arguments, can extract data such as filepath, mutex, main linkedlist
    Args *args = (Args*)in;
    //checks if directory is accessible, if not returns an error
	DIR *d;

    d = opendir(args->pathName);

    if(d==NULL)
    {
        printf("There was an error with the directory at: %s", args->pathName);
	   //perror(d);
        return NULL;
    }

    //create list of threads for each item unearthed in this directory
    Node threads = {NULL, NULL};
    //store pointer to the head for later

    //iterate through every item in the directory. Two cases: file or directory
    struct dirent *dp;
    while((dp = readdir(d)) != NULL)
    {
		Args *a1 = malloc(sizeof(struct Arguments));
        a1->listHead = args->listHead;
        a1->mut = args->mut;
        a1->pathName = pathGenerator(args->pathName, dp->d_name);
        pthread_t t1;
		
        if(dp->d_type==DT_DIR)
        {
            //Create thread with directoryHandler function for the directory found.
            //Pass along the mutex, the main linked list, and the updated path, in a new struct
            //Adds this thread to the linkedlist of threads associated with this particular function call
			if(strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") !=0)
			{
				pthread_create(&t1, NULL, &directoryHandler, a1);
				add(threads, t1);
			}
        }
        else
        {
            //Create thread with fileHandler function for the directory found.
            //Pass along the mutex, the main linked list, and the updated path, in a new struct
            //Adds this thread to the linkedlist of threads associated with this particular function call

            pthread_create(&t1, NULL, &fileHandler, a1);
			add(threads, t1);

        }

    }
    //go through list and join threads
	Iterator iter = {&threads};
    while(hasNext(&iter))
    {
        pthread_join(*(pthread_t*)nextData(&iter), NULL);
    }
    closedir(d);
	deleteList(&threads, 0);
	
	return NULL;
}

int listLength(Node* in)
{
    Node* list = (Node*)in->data;
    int count = 0;
    while(list!=NULL)
    {
        count++;
        list = list->next;
    }
    return count;
}

double analyzePair(Node* in1, Node* in2)
{
	Node* meanConstruct = malloc(sizeof(Node));
	Node* token1 = in1->next;
	Node* token2 = in2->next;
	while(token1!=NULL)
	{
		NameData* data1 = (NameData*)token1->data;
		double freq1 = data1->freq;
		double freq2 = findTokenName(token2, data1->name);
		
		NameData construct = {data1->name, (freq1+freq2)/2};
		nodeAdd(meanConstruct, &construct, 3);
		
		token1 = token1->next;
	}
	token1 = in1->next;
	while(token2!=NULL)
	{
		NameData* data2 = (NameData*)token2->data;
		double freq2 = data2->freq;
		double freq1 = findTokenName(token1, data2->name);
		//printf("GENERIC TOKEN: %s\n", data2->name);
		if(freq1 == 0)
		{
			//printf("NON DUPLICATE TOKEN: %s\n", data2->name);
			NameData construct = {data2->name, (freq2)/2};
			nodeAdd(meanConstruct, &construct, 3);
		}
		token2 = token2->next;
	}
	/*Node* meanConstructTestPointer = meanConstruct;
	while(meanConstructTestPointer!=NULL)
	{
		printf("Token name, mean freq: %s %f\n", ((NameData*)meanConstructTestPointer->data)->name, ((NameData*)meanConstructTestPointer->data)->freq);
		meanConstructTestPointer = meanConstructTestPointer->next;
	}*/

	double kld1 = 0;
	double kld2 = 0;
	token1 = in1->next;
	while(token1!=NULL)
	{
		NameData* data1 = (NameData*)token1->data;
		double freq1 = data1->freq;
		double mean1 = findTokenName(meanConstruct, data1->name);
		double loggy = log10(freq1/mean1);
		kld1 += (freq1 * loggy);
		token1 = token1->next;
	}
	/*printf("kld1: %f\n", kld1);

	char* test = malloc(sizeof(char));
	*test = 'a';

	printf("frequency of a in meanConstruct: %f\n", findTokenName(meanConstruct, test));*/


	
	token2 = in2->next;
	while(token2!=NULL)
	{	
		NameData* data2 = (NameData*)token2->data;
		double freq2 = data2->freq;
		//printf("freq2: %f\n", freq2);
		double mean2 = findTokenName(meanConstruct, data2->name);
		//printf("name: %s\n", data2->name);
		//printf("mean2: %f\n", mean2);
		double loggy = log10(freq2/mean2);
		//printf("loggy: %f\n", loggy);
		kld1 += (freq2 * loggy);
		token2 = token2->next;
	}
	//printf("kld2: %f\n", kld1);

	return (kld1+kld2)/2;
}

void analyze(Node* in)
{
	Node* start = in;
	while(start!=NULL && start->next!=NULL)
	{
		Node* current = start->next;
		while(current!=NULL)
		{
			Node* p1 = (Node*)start->data;
			Node* p2 = (Node*)current->data;
			double result = analyzePair(p1, p2);
			if(result<=0.1)
			{
				printf(ANSI_COLOR_RED     "%f"     ANSI_COLOR_RESET "", result);
			}
			else if(result<=0.15)
			{
				printf(ANSI_COLOR_YELLOW    "%f"     ANSI_COLOR_RESET "", result);
			}
			else if(result<=0.2)
			{
				printf(ANSI_COLOR_GREEN    "%f"     ANSI_COLOR_RESET "", result);
			}
			else if(result<=0.25)
			{
				printf(ANSI_COLOR_CYAN    "%f"     ANSI_COLOR_RESET "", result);
			}
			else if(result<=0.3)
			{
				printf(ANSI_COLOR_BLUE    "%f"     ANSI_COLOR_RESET "", result);
			}
			else if(result>0.3)
			{
				printf(ANSI_COLOR_WHITE   "%f"     ANSI_COLOR_RESET "", result);
			}
			NameData* data1 = (NameData*)p1->data;
			NameData* data2 = (NameData*)p2->data;
			printf(" \"%s\" and \"%s\" \n", data1->name, data2->name);
			current = current->next;
		}
		start = start->next;
	}
}

void printTest(Node* in)
{
    Node* current = in;
    while(current!=NULL && current->data != NULL)
    {
        Node* tokens = (Node*)current->data;
        NameData* info = (NameData*)tokens->data;
        printf("File name: %s\n", info->name);
        printf("Word count: %f\n", info->freq);
		tokens = tokens->next;
		while(tokens!=NULL)
		{
			NameData* info = (NameData*)tokens->data;
			printf("Token: %s\n", info->name);
       		printf("Frequency: %f\n", info->freq);
			tokens = tokens->next;
		}
		printf("\n");
        current = current->next;
    }
}

int main(int argc, char *argv[])
{
    //call a thread with the following arguments for the directory handler funciton:
    Args *a = malloc(sizeof(Args));
    Node *bigList = malloc(sizeof(Node));
    bigList->data = NULL;
    bigList->next = NULL;
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    a->pathName = argv[1];  
    a->listHead = bigList;
    a->mut = &lock;
    pthread_t t;

    pthread_create(&t, NULL, &directoryHandler, a);
    pthread_join(t, NULL);
	//printTest(bigList);
	
    //Pre-Analysis
    if(bigList->data == NULL)
    {
        printf("No data written");
        return EXIT_SUCCESS;
    }
    if(listLength(bigList)==1)
    {
        printf("Warning: only one entry");
		return EXIT_SUCCESS;
    }
	//printf("analyzing\n");
	analyze(bigList);
}