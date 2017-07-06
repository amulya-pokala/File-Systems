#define _CRT_SECURE_NO_WARNINGS
#include<io.h>
#include<pthread.h>
#include<stdio.h>
#include<winsock2.h> //Winsock Library
#include <stdlib.h>
#include<string.h>
pthread_mutex_t lock;
#pragma comment(lib,"ws2_32.lib")
WSADATA wsa;
SOCKET s, new_socket;
FILE *fp, *fp1;
struct sockaddr_in server, client;
typedef struct fileMetaData FMData;
typedef struct CNode cnode;
typedef struct messageNode MNode;
int count,is_updated=0,cache_count,cache_count1;
struct CNode
{
	char filename[20];
	char messages[100][128];
	int num_msgs;
};
cnode cache[16];
struct messageNode
{
	int start_ofset;
	int end_ofset;
};

struct fileMetaData
{
	char fileName[20];
	int file_ofset;
	MNode messageOfsetArray[100];
	int num_msgs;
	char author[20];
	int file_size;
};
FMData head[100];
void retrieveContents(FILE *file_file,FILE *message_file)
{
	FILE *fp3 = fopen("files.dat", "r+");
	int i, count1;
	long message, file;
	count = 0;
	fread(&message, sizeof(message), 1, fp3);
	fread(&file, sizeof(file), 1, fp3);
	if (file == 0) file = 1048576;
	if (message == 0) message = 12280;
	fseek(file_file, file, SEEK_SET);
	fseek(message_file, message, SEEK_SET);
	fread(&count1, sizeof(count1), 1, fp3);
	printf("count: %d\n", count1);
	for (i = 0; i<count1; i++)
	{
		fread(&head[i], sizeof(FMData), 1, fp3);
		count++;
	}
	int flag = 1;
	send(new_socket, (char *)&flag, sizeof(flag), 0);
	send(new_socket, (char *)&count1, sizeof(count), 0);
	int j;
	FILE *f = fopen("files.dat", "r");
	char n[128];
	for (i = 0; i < count1; i++)
	{
		int size = sizeof(head[i]);
		printf("size is: %d\n", size);
		send(new_socket, (char *)&size, sizeof(size), 0);
		send(new_socket, (char *)&head[i], size, 0);
		for (j = 0; j < head[i].num_msgs; j++)
		{
			long ofset = (head[i]).messageOfsetArray[j].start_ofset;
			fseek(f, ofset,SEEK_SET);
			fread(&n, 128, 1, f);
			send(new_socket, (char *)&n, sizeof(n), 0);
		}

	}
	fclose(fp3);
}

int searchInCache(char *filename)
{
	int i = 0;
	for (i = 0; i<cache_count; i++)
	{
		if (strcmp(cache[i].filename, filename) == 0)
		{
			int j = 0;
			printf("\nmessages\n");
			for (j = 0; j<cache[i].num_msgs; j++)
			{
				printf("%s\n", cache[i].messages[j]);
			}
			return 1;
		}
	}
	return 0;
}
void addIntoCache(int index)
{
	if (cache_count<16)
	{

		strcpy(cache[cache_count].filename, head[index].fileName);
		int j;
		FILE *f5 = fp1;
		char name[128];
		for (j = 0; j<head[index].num_msgs; j++)
		{
			fseek(f5, head[index].messageOfsetArray[j].start_ofset, SEEK_SET);
			fread(&name, 128, 1, f5);
			strcpy(cache[cache_count].messages[cache[cache_count].num_msgs], name);
			cache[cache_count].num_msgs++;
		}
		cache_count++;
	}
	else{

		if (cache_count1 >= 16) cache_count1 = 0;
		strcpy(cache[cache_count1].filename, head[index].fileName);
		int j;
		FILE *f5 = fp1;
		char name[128];
		for (j = 0; j<head[index].num_msgs; j++)
		{
			fseek(f5, head[index].messageOfsetArray[j].start_ofset, SEEK_SET);
			fread(&name, 128, 1, f5);
			strcpy(cache[cache_count1].messages[cache[cache_count1].num_msgs], name);
			cache[cache_count1].num_msgs++;
		}
		cache_count1++;
	}
}
void* uploadFile(void *f2)
{
	
	printf("enter the file name\n");
	char fileName[20];
	char author[20];
	scanf("%s", fileName);
	int ofset = ftell(fp);
	char c1,data[102400];
	int ind = 0;
	FILE *f = fopen(fileName, "r");
	while ((c1 = getc(f)) != EOF)
	{
		data[ind++] = c1;
		//fputc(c1, fp);
	}
	data[ind] = c1;
	fwrite(&data, 102400, 1, fp);
	//fputc(c1, fp);
	head[count].file_size = ind;
	fclose(f);
	//pthread_mutex_lock(&lock);
	head[count].file_ofset = ofset;
	printf("enter author name\n");
	scanf("%s", author);
	strcpy(head[count].fileName, fileName);
	strcpy(head[count].author, author);
	int type = 1;
	send(new_socket, (char *)&type, sizeof(type), 0);
	send(new_socket, (char *)&head[count], sizeof(head[count]), 0);
	//send(new_socket, (char *)&data, sizeof(data), 0);
	count++;
	//pthread_mutex_unlock(&lock);
	printf("\n--------------------------------------------\n");
	return NULL;
}
void* updateFile(void *q)
{
	char author[20];
	char fileName[20];
	printf("enter the filename\n");
	scanf("%s", fileName);
	printf("enter the author name\n");
	scanf("%s", author);
	int i;
	pthread_mutex_lock(&lock);
	for (i = 0; i<count; i++)
	{
		if (strcmp(head[i].fileName, fileName) == 0)
			break;
	}
	pthread_mutex_lock(&lock);
	int ofset = head[i].file_ofset;
	FILE *fp5 = fopen("files.dat", "r+");
	fseek(fp5, ofset, SEEK_SET);
	FILE *f = fopen(fileName, "r");
	char c1;
	while ((c1 = getc(f)) != EOF)
	{
		fputc(c1, fp5);
	}
	strcpy(head[i].author, author);
	fclose(fp5);
	fclose(f);
	return NULL;
}
void* addMessagesToFile(void *f)
{
	printf("enter the filename into which you want to write the messages into\n");
	char filename[20];
	scanf("%s", filename);
	int i;
	pthread_mutex_lock(&lock);
	for (i = 0; i<count; i++) if (strcmp(head[i].fileName, filename) == 0) break;
	pthread_mutex_unlock(&lock);
	printf("enter message\n");
	char message[128];
	int start_ofset = ftell(fp1);
	scanf("%s", message);
	printf("start ofset: %d\n", start_ofset);
	fwrite(&message, 1, 128, fp1);
	int end_ofset = ftell(fp1);
	printf("end ofset: %d\n", end_ofset);
	head[i].messageOfsetArray[head[i].num_msgs].start_ofset = start_ofset;
	head[i].messageOfsetArray[head[i].num_msgs].end_ofset = end_ofset;
	head[i].num_msgs++;
	int k = 0;
	for (k = 0; k<cache_count; k++)
	{
		if (strcmp(cache[k].filename, filename) == 0)
		{
			strcpy(cache[k].messages[cache[k].num_msgs], message);
			cache[k].num_msgs++;
			break;
		}
	}
	int type = 4;
	send(new_socket, (char *)&type, sizeof(type), 0);
	send(new_socket, (char *)&start_ofset, sizeof(start_ofset), 0);
	send(new_socket, (char *)&message, sizeof(message), 0);
	send(new_socket, (char *)&filename, sizeof(filename), 0);
	return NULL;
}
void* viewMessages(void *f)
{
	printf("enter the filename for which you want the messages for\n");
	char filename[20];
	scanf("%s", filename);
	int set = searchInCache(filename);
	if (set == 0)
	{
		int i;
		for (i = 0; i<count; i++)
		{
			if (strcmp(head[i].fileName, filename) == 0)
			{
				addIntoCache(i);
				break;
			}
		}
		searchInCache(filename);
	}
	return NULL;
}
void* editMessage(void *q)
{
	FILE *fp4 = fopen("files.dat", "r+");
	printf("enter the file name");
	char fileName[20], name[128];
	scanf("%s", fileName);
	printf("enter the message you want to edit\n");
	char message[128],message1[128];
	scanf("%s", message);
	int i, j,ofset;
	pthread_mutex_lock(&lock);
	for (i = 0; i<count; i++)
	{
		if (strcmp(head[i].fileName, fileName) == 0) break;
	}
	pthread_mutex_unlock(&lock);
	for (j = 0; j<head[i].num_msgs; j++)
	{
		fseek(fp4, head[i].messageOfsetArray[j].start_ofset, SEEK_SET);
		fread(&name, 1, 128, fp4);
		if (strcmp(name, message) == 0)
		{
			fseek(fp4, head[i].messageOfsetArray[j].start_ofset, SEEK_SET);
			ofset = head[i].messageOfsetArray[j].start_ofset;
			printf("enter the new message\n");

			scanf("%s", message1);
			fwrite(&message, 1, 128, fp4);
		}
	}
	int k, flag = 0;
	for (k = 0; k<cache_count; k++)
	{
		if (strcmp(cache[k].filename, fileName) == 0)
		{
			int g;
			for (g = 0; g<cache[k].num_msgs; g++)
			{
				if (strcmp(cache[k].messages[g], message))
				{
					strcpy(cache[k].messages[g], message1);
					flag = 1;
					break;
				}
			}
			if (flag == 1) break;
		}
	}
	int type = 3;
	send(new_socket, (char *)&type, sizeof(type), 0);
	send(new_socket, (char *)&fileName, sizeof(fileName), 0);
	send(new_socket, (char *)&ofset, sizeof(ofset), 0);
	send(new_socket, (char *)&message1, sizeof(message1), 0);
	fclose(fp4);
	return NULL;
}

void* downloadFile(void *p)
{
	printf("enter the file name\n");
	char filename[20];
	scanf("%s", filename);
	FILE *f1 = fopen("files.dat", "r+");
	FILE *f = fopen(filename, "wb");
	int i;
	for (i = 0; i<count; i++)
	{
		if (strcmp(head[i].fileName, filename) == 0)
		{
			int ofset = head[i].file_ofset;
			char data[102400];
			fread(&data, 102400, 1, f1);
			int ind = 0, in = head[i].file_size;
			printf("file size: %d\n", in);
			while (ind <= in)
			{
				fputc(data[ind++], f);
			}
			break;
		}
	}
	return NULL;
}

int power(int a, int b)
{
	int pow = 1, i;
	for (i = 0; i<b; i++) pow = pow*a;
	return pow;
}
void* listen1(void *f)
{
	int recv_size;
	int type;
	printf("\n\n\n\in listen\n\n\n\n");
	while ((recv_size = recv(new_socket, (char *)&type, sizeof(type), 0)) == SOCKET_ERROR)
	{
		//	puts("recv failed");
	}
	printf("\n\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\n");
	printf("type: %d\n", type);
	char filename[20], author[20], file_ofset;
	if (type == 1)
	{
		FMData f;
		if ((recv_size = recv(new_socket, (char *)&f, sizeof(f), 0)) == SOCKET_ERROR)
		{
			puts("recv failed");
		}
		char data[102400];
		if ((recv_size = recv(new_socket, (char *)&data, sizeof(data), 0)) == SOCKET_ERROR)
		{
			puts("recv failed");
		}
		//pthread_mutex_lock(&lock);
		strcpy(head[count].fileName, f.fileName);
		strcpy(head[count].author, f.author);
		head[count].file_ofset = f.file_ofset;
		head[count].file_size = f.file_size;
		FILE *f1 = fopen(f.fileName, "w");
		int i = 0;
		for (; data[i] != EOF && i < 102400; i++)
		{
			fputc(data[i], f1);
		}
		fclose(f1);
		printf("filename :%s\n", head[count].fileName);
		printf("filename :%s\n", head[count].author);
		printf("filename :%d\n", head[count].file_ofset);
		count++;
		//pthread_mutex_unlock(&lock);

	}
	if (type == 2)
	{
		FMData f;
		if ((recv_size = recv(new_socket, (char *)&f, sizeof(f), 0)) == SOCKET_ERROR)
		{
			puts("recv failed");
		}
		//pthread_mutex_lock(&lock);
		//pthread_mutex_lock(&lock);
		strcpy(head[count].fileName, f.fileName);
		strcpy(head[count].author, f.author);
		head[count].file_ofset = f.file_ofset;
		printf("filename :%s\n", head[count].fileName);
		printf("filename :%s\n", head[count].author);
		printf("filename :%d\n", head[count].file_ofset);
		//pthread_mutex_unlock(&lock);
	}
	if (type == 4)
	{
		int ofset;
		if ((recv_size = recv(new_socket, (char *)&ofset, sizeof(ofset), 0)) == SOCKET_ERROR)
		{
			puts("recv failed");
		}
		char message[128], filename[20];
		if ((recv_size = recv(new_socket, (char *)&message, sizeof(message), 0)) == SOCKET_ERROR)
		{
			puts("recv failed");
		}
		if ((recv_size = recv(new_socket, (char *)&filename, sizeof(filename), 0)) == SOCKET_ERROR)
		{
			puts("recv failed");
		}
		int i;
		//pthread_mutex_lock(&lock);
		for (i = 0; i < count; i++)
		{
			if (strcmp(head[i].fileName, filename) == 0)
			{
				head[i].messageOfsetArray[head[i].num_msgs].start_ofset = ofset;
				head[i].num_msgs++;
				break;
			}
		}
		//pthread_mutex_unlock(&lock);
		printf("\n\n-----------------------------------\n\n");
		FILE *f = fopen("files.dat", "r+");
		fseek(f, ofset, SEEK_SET);
		fwrite(&message, 1, 128, f);
		fclose(f);
		printf("\n\ndone\n\n");
	}
	if (type == 5)
	{
		char filename[20], message[128];
		int ofset;
		if ((recv_size = recv(s, (char *)&filename, sizeof(filename), 0)) == SOCKET_ERROR)
		{
			puts("recv failed");
		}
		if ((recv_size = recv(s, (char *)&ofset, sizeof(ofset), 0)) == SOCKET_ERROR)
		{
			puts("recv failed");
		}
		int i;
		for (i = 0; i < 128; i++) message[i] = '\0';
		FILE *fp5 = fopen("files.dat", "r+");
		fseek(fp5, ofset, SEEK_SET);
		fwrite(&message, 1, 128, fp5);
		fclose(fp5);
	}
	if (type == 3)
	{
		char filename[20], message[128];
		int ofset;
		if ((recv_size = recv(new_socket, (char *)&filename, sizeof(filename), 0)) == SOCKET_ERROR)
		{
			puts("recv failed");
		}
		if ((recv_size = recv(new_socket, (char *)&ofset, sizeof(ofset), 0)) == SOCKET_ERROR)
		{
			puts("recv failed");
		}
		if ((recv_size = recv(new_socket, (char *)&message, sizeof(message), 0)) == SOCKET_ERROR)
		{
			puts("recv failed");
		}
		FILE *f = fopen("files.dat", "r+");
		fseek(f, ofset, SEEK_SET);
		fwrite(&message, 1, 128, fp);
		fclose(f);
	}
	void *q = NULL;
	listen1(q);
	return NULL;
}
void* deleteFile(void *p)
{
	printf("enter file name\n");
	char filename[20];
	scanf("%s", filename);
	int i;
	for (i = 0; i < count; i++)
	{
		if (strcmp(head[i].fileName, filename) == 0)
		{
			break;
		}
	}
	int ofset = head[i].file_ofset;
	FILE *f = fopen("files.dat", "r+");
	fseek(f, ofset, SEEK_SET);
	int o = ofset + 102400;
	for (i = ofset; i < o; i++) fputc('\0', f);
	fclose(f);
	return NULL;
}
void *deleteMessage(void *p)
{
	printf("enter filename\n");
	char filename[20];
	scanf("%s", filename);
	printf("enter the message to delete\n");
	char message[128];
	FILE *f = fopen("files.dat", "r+");
	scanf("%s", message);
	char name[128];
	char replace[128];
	int i,ofset;
	for (i = 0; i < 128; i++) replace[i] = '\0';
	int flag = 0;
	for (i = 0; i < count; i++)
	{
		if (strcmp(head[i].fileName, filename) == 0)
		{
			int j;
			for (j = 0; j < head[i].num_msgs; j++)
			{
				fseek(f,head[i].messageOfsetArray[j].start_ofset,SEEK_SET);
				fread(&name, 1, 128, f);
				if (strcmp(name, message) == 0)
				{
					ofset = head[i].messageOfsetArray[j].start_ofset;
					fseek(f, head[i].messageOfsetArray[j].start_ofset, SEEK_SET);
					fwrite(&replace, 1, 128, f);
					flag = 1;
					break;
				}
			}
			if (flag == 1) break;
		}
	}
	int type = 5;
	send(new_socket, (char *)&type, sizeof(type), 0);
	send(new_socket, (char *)&filename, sizeof(filename), 0);
	send(new_socket, (char *)&ofset, sizeof(ofset), 0);
	return NULL;
}
int main()
{
	if (pthread_mutex_init(&lock, NULL) != 0)
	{
		printf("\n mutex init failed\n");
		return 1;
	}
	printf("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		return 1;
	}

	printf("Initialised.\n");

	//Create a socket
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		printf("Could not create socket : %d", WSAGetLastError());
	}

	printf("Socket created.\n");

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(8887);

	//Bind
	if (bind(s, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
	{
		printf("Bind failed with error code : %d", WSAGetLastError());
	}

	puts("Bind done");

	//Listen to incoming connections
	listen(s, 3);

	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	int c1;
	c1 = sizeof(struct sockaddr_in);
	new_socket = accept(s, (struct sockaddr *)&client, &c1);
	if (new_socket == INVALID_SOCKET)
	{
		printf("accept failed with error code : %d", WSAGetLastError());
	}

	puts("Connection accepted");
	char c = 'y';
	int choice;
	fp = fopen("files.dat", "r+");
	fseek(fp, 1048576, SEEK_SET);
	fp1 = fopen("files.dat", "r+");
	fseek(fp1, 12280, SEEK_SET);
	pthread_t thread1, thread2;
	void *q = NULL;
	pthread_create(&thread1, NULL, listen1, &q);
	int count_mark = 0;
	while (c == 'y')
	{
		printf("0-Retrieve contents from metadata\n1-Upload a file\n2-Update a file\n3-Delete a file\n4-Add messages to a file\n5-Delete a message\n6-Edit a message\n7-View Messages\n8-Download file\n9-Delete a message\n10-Delete a file\n");
		scanf("%d", &choice);
		if (choice == 0)
		{
			count_mark++;
			retrieveContents(fp, fp1);
			
		}

		else if (choice == 1)
		{
			if (count_mark == 0)
			{
				int flag = 0;
				send(new_socket, (char *)&flag, sizeof(flag), 0);
				count_mark++;
			}
			pthread_create(&thread2, NULL, uploadFile, &q);
			pthread_join(thread2, NULL);
		}
		else if (choice == 2)
		{
			if (count_mark == 0)
			{
				int flag = 0;
				send(new_socket, (char *)&flag, sizeof(flag), 0);
				count_mark++;
			}
			pthread_create(&thread2, NULL, updateFile, &q);
			pthread_join(thread2, NULL);
		}
		else if (choice == 4)
		{
			if (count_mark == 0)
			{
				int flag = 0;
				send(new_socket, (char *)&flag, sizeof(flag), 0);
				count_mark++;
			}
			pthread_create(&thread2, NULL, addMessagesToFile, &q);
			pthread_join(thread2, NULL);
		}
		else if (choice == 7)
		{
			if (count_mark == 0)
			{
				int flag = 0;
				send(new_socket, (char *)&flag, sizeof(flag), 0);
				count_mark++;
			}

			pthread_create(&thread2, NULL, viewMessages, &q);
			pthread_join(thread2, NULL);
		}
		else if (choice == 6)
		{
			if (count_mark == 0)
			{
				int flag = 0;
				send(new_socket, (char *)&flag, sizeof(flag), 0);
				count_mark++;
			}
			pthread_create(&thread2, NULL, editMessage, &q);
			pthread_join(thread2, NULL);
		}
		else if (choice == 8)
		{
			if (count_mark == 0)
			{
				int flag = 0;
				send(new_socket, (char *)&flag, sizeof(flag), 0);
				count_mark++;
			}
			pthread_create(&thread2, NULL, downloadFile, &q);
			pthread_join(thread2, NULL);
		}
		else if (choice == 9)
		{
			if (count_mark == 0)
			{
				int flag = 0;
				send(new_socket, (char *)&flag, sizeof(flag), 0);
				count_mark++;
			}
			pthread_create(&thread2, NULL, deleteMessage, &q);
			pthread_join(thread2, NULL);
		}
		else if (choice == 10)
		{
			if (count_mark == 0)
			{
				int flag = 0;
				send(new_socket, (char *)&flag, sizeof(flag), 0);
				count_mark++;
			}
			pthread_create(&thread2, NULL, deleteFile, &q);
			pthread_join(thread2, NULL);
		}
		printf("if you want to do more operations, press y\n");
		scanf("%c", &c);
		scanf("%c", &c);
	}
	
	long message_ends = ftell(fp1);
	long file_ends = ftell(fp);
	FILE *fp2 = fopen("files.dat", "r+");
	fseek(fp2, 0, SEEK_SET);
	fwrite(&message_ends, sizeof(message_ends), 1, fp2);
	fwrite(&file_ends, sizeof(file_ends), 1, fp2);
	fwrite(&count, sizeof(count), 1, fp2);
	int i;
	printf("coming here\n");
	for (i = 0; i<count; i++)
	{
		fwrite(&(head[i]), sizeof(head[i]), 1, fp2);
	}
	fclose(fp);
	fclose(fp1);
	fclose(fp2);
	pthread_join(thread1, NULL);
	getchar();
	return 0;
}
