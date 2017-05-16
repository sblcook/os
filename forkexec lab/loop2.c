#include<stdio.h>
#include<sys/time.h>

int main(void) {

	struct timeval begin, end;
	gettimeofday(&end, NULL);	
	long endTime = end.tv_usec;
	printf("End time: %ld\n", endTime);

	int i = 0;
	int j = 0;
	int depth = 10;
	int length = 121;
	char data[depth][length+1];

	for(i = 0; i < depth; i++){
		for(j = 0; j < length - 1; j++){
			data[i][j] = 'A' + (random() % 26);

		}
		
		data[i][length - 1] = '\0';//null terminated string
		
	}


	FILE *f = fopen("contents2.txt", "w+");//opens contents.txt
	for(i = 0; i < depth; i++){//writes a random string of 121 chars, the last being the null symbol
		fputs(data[i], f);
	}
		
	fclose(f);
		

	int lineNumber = random() % 10;//sets a random line, or "record", to be read
	long int offset = 121 * lineNumber;//sets offset based on line
	char readData[121];//line size, 120 incl null terminus

	FILE *fp = fopen("contents2.txt", "r");//opens the file with the written text
	if (fp != NULL){ //makes sure the file exists
		fseek(fp, offset, SEEK_SET);//moves cursor to offset defined above
		fgets(readData, 121, fp);//from the cursor, stores 121 chars in readData string
	}

	strcmp(readData, data[lineNumber]);//compares the read data, in readData, to the written data in data[]

	fclose(fp);
		
}


