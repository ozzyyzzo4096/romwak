/* ROMWak (ANSI C port by freem, original by Jeff Kurtz)
 *
 * ROMWak was originally programmed in Delphi. This port is in ANSI C.
 * The sources given to me were of version 0.3, though I have a version 0.4
 * binary on my computer. I have not analyzed the differences between the two
 * versions, as I do not have access to Delphi build tools.
 */
#include <stdio.h>
#include <stdlib.h>

#include "romwak.h"

#define ROMWAK_VERSION	"0.7" /* derived from 0.4 source code; see above note */


#define EIGHT_MB (8*1024*1024)

/* Usage() - Print program usage. */
void Usage(){
	printf("usage: romwak <option> <infile> <outfile> [outfile2] [psize] [pbyte]\n");
	printf("You must use one of these options:\n");
	printf(" /b - Split file into two files, alternating bytes into separate files.\n");
	printf(" /c - Concatenate two files : <infile1> <infile2> <outfile>\n");
	printf(" /d - Darksoft concatenate crom files : <infile1> <infile2> <outfile>\n");
	printf(" /e - Darksoft concatenate prom files : <infile1> <infile2> <outpath>\n");
	printf(" /f - Flip low/high bytes of a file. (<outfile> optional.)\n");
	printf(" /h - Split file in half (two files).\n");
	printf(" /i - Generate rom information (size,crc) (as a text file).\n");
	printf(" /m - Byte merge two files. (stores results in <outfile2>).\n");
	printf(" /q - Byte merge four files. (See readme for syntax)\n");
	printf(" /s - Swap top and bottom halves of a file. (<outfile2> optional.)\n");
	printf(" /u - Byte update two files. (stores results in <outfile2>).\n");
	printf(" /w - Split file into two files, alternating words into output files.\n");
	printf(" /p - Pad file to [psize] in K with [pbyte] value (0-255).\n");
	printf("\n");
	printf("NOTE: Omission of [outfile2] will result in the second file not being saved.\n");
	printf("\n");
	printf("See the included README.md for more details. If README.md was not included,\n");
	printf("please visit https://github.com/freem/romwak/\n");
}
/*----------------------------------------------------------------------------*/

/* FileExists(char *fileIn) - Helper function to determine if a file exists.
 *
 * (Params)
 * char *fileIn			Target filename
 */
bool FileExists(char *fileIn){
	/* try opening file */
	FILE *f = fopen(fileIn,"r");
	if(f != NULL){
		/* file open was successful, we don't need this handle. */
		fclose(f);
		return true;
	}
	/* file open unsuccessful, print error */
	perror("Error attempting to open file");
	return false;
}
/*----------------------------------------------------------------------------*/

/* FileSize(FILE *pFile) - Helper function to determine a file's size.
 * Does not rewind the file, you must do it yourself.
 *
 * (Params)
 * FILE *pFile			Handle to a loaded file.
 */
long FileSize(FILE *pFile){
	fseek(pFile,0,SEEK_END); /* SEEK_END is non-portable, sorry */
	return ftell(pFile);
}
/*----------------------------------------------------------------------------*/

/* EqualSplit(char *fileIn, char *fileOutA, char *fileOutB) - /h
 * Splits a file in half equally.
 * (Get filesize, divide it by 2, and split the data. Easy enough.)
 *
 * (Params)
 * char *fileIn			Input filename
 * char *fileOutA		Output filename 1
 * char *fileOutB		Output filename 2
 */
int EqualSplit(char *fileIn, char *fileOutA, char *fileOutB){
	FILE *pInFile, *pOutFile1, *pOutFile2;
	long length;
	unsigned char *inBuffer;
	size_t result;
	long halfLength;
	unsigned char *outBuf1;
	unsigned char *outBuf2;
	long i = 0;
	long pos = 0;
	unsigned char b1, b2;

	if(!FileExists(fileIn)){
		return EXIT_FAILURE;
	}
	printf("Splitting file '%s' equally, saving to '%s' and '%s'\n",fileIn,fileOutA,fileOutB);

	pInFile = fopen(fileIn,"rb");
	if(pInFile == NULL){
		perror("Error attempting to open input file");
		exit(EXIT_FAILURE);
	}

	/* find file size */
	length = FileSize(pInFile);
	rewind(pInFile);

	/* copy data into buffer */
	inBuffer = (unsigned char*)malloc(length);
	if(inBuffer == NULL){
		printf("Error allocating memory for input file buffer.");
		exit(EXIT_FAILURE);
	}

	result = fread(inBuffer,sizeof(unsigned char),length,pInFile);
	if(result != length){
		perror("Error reading input file");
		exit(EXIT_FAILURE);
	}
	fclose(pInFile);

	/* prepare output buffers */
	halfLength = length/2;
	outBuf1 = (unsigned char*)malloc(halfLength);
	if(outBuf1 == NULL){
		printf("Error allocating memory for first output file buffer.");
		exit(EXIT_FAILURE);
	}
	outBuf2 = (unsigned char*)malloc(halfLength);
	if(outBuf2 == NULL){
		printf("Error allocating memory for second output file buffer.");
		exit(EXIT_FAILURE);
	}

	/* fill buffers */
	while(i<halfLength){
		b1 = inBuffer[i];
		b2 = inBuffer[i+halfLength];
		outBuf1[pos] = b1;
		outBuf2[pos] = b2;
		i++;
		pos++;
	}
	free(inBuffer);

	/* write output files */
	pOutFile1 = fopen(fileOutA,"wb");
	if(pOutFile1 == NULL){
		perror("Error attempting to create first output file");
		exit(EXIT_FAILURE);
	}
	result = fwrite(outBuf1,sizeof(unsigned char),halfLength,pOutFile1);
	if(result != halfLength){
		perror("Error writing first output file");
		exit(EXIT_FAILURE);
	}
	fclose(pOutFile1);
	printf("'%s' saved successfully!\n",fileOutA);
	free(outBuf1);

	pOutFile2 = fopen(fileOutB,"wb");
	if(pOutFile2 == NULL){
		perror("Error attempting to create second output file");
		exit(EXIT_FAILURE);
	}
	result = fwrite(outBuf2,sizeof(unsigned char),halfLength,pOutFile2);
	if(result != halfLength){
		perror("Error writing second output file");
		exit(EXIT_FAILURE);
	}
	fclose(pOutFile2);
	printf("'%s' saved successfully!\n",fileOutB);
	free(outBuf2);

	return EXIT_SUCCESS;
}
/*----------------------------------------------------------------------------*/

/* ByteSplit(char *fileIn, char *fileOutA, char *fileOutB) - /b
 *
 * (Params)
 * char *fileIn			Input filename
 * char *fileOutA		Output filename 1
 * char *fileOutB		Output filename 2
 */
int ByteSplit(char *fileIn, char *fileOutA, char *fileOutB){
	FILE *pInFile, *pOutFile1, *pOutFile2;
	long length;
	unsigned char *inBuffer;
	size_t result;
	long halfLength;
	unsigned char *outBuf1;
	unsigned char *outBuf2;
	long i = 0;
	long pos1 = 0;
	long pos2 = 1;

	if(!FileExists(fileIn)){
		return EXIT_FAILURE;
	}
	printf("Splitting file '%s' into bytes, saving to '%s' and '%s'\n",fileIn,fileOutA,fileOutB);

	pInFile = fopen(fileIn,"rb");
	if(pInFile == NULL){
		perror("Error attempting to open input file");
		exit(EXIT_FAILURE);
	}

	/* find file size */
	length = FileSize(pInFile);
	rewind(pInFile);

	/* copy data into buffer */
	inBuffer = (unsigned char*)malloc(length);
	if(inBuffer == NULL){
		printf("Error allocating memory for input file buffer.");
		exit(EXIT_FAILURE);
	}

	result = fread(inBuffer,sizeof(unsigned char),length,pInFile);
	if(result != length){
		perror("Error reading input file");
		exit(EXIT_FAILURE);
	}
	fclose(pInFile);

	/* prepare output buffers */
	halfLength = length/2;
	outBuf1 = (unsigned char*)malloc(halfLength);
	if(outBuf1 == NULL){
		printf("Error allocating memory for first output file buffer.");
		exit(EXIT_FAILURE);
	}
	outBuf2 = (unsigned char*)malloc(halfLength);
	if(outBuf2 == NULL){
		printf("Error allocating memory for second output file buffer.");
		exit(EXIT_FAILURE);
	}

	/* fill buffers */
	while(i<halfLength){
		outBuf1[i] = inBuffer[pos1];
		outBuf2[i] = inBuffer[pos2];
		i++;
		pos1+=2;
		pos2+=2;
	}
	free(inBuffer);

	/* write output files */
	pOutFile1 = fopen(fileOutA,"wb");
	if(pOutFile1 == NULL){
		perror("Error attempting to create first output file");
		exit(EXIT_FAILURE);
	}
	result = fwrite(outBuf1,sizeof(unsigned char),halfLength,pOutFile1);
	if(result != halfLength){
		perror("Error writing first output file");
		exit(EXIT_FAILURE);
	}
	fclose(pOutFile1);
	printf("'%s' saved successfully!\n",fileOutA);
	free(outBuf1);

	pOutFile2 = fopen(fileOutB,"wb");
	if(pOutFile2 == NULL){
		perror("Error attempting to create second output file");
		exit(EXIT_FAILURE);
	}
	result = fwrite(outBuf2,sizeof(unsigned char),halfLength,pOutFile2);
	if(result != halfLength){
		perror("Error writing second output file");
		exit(EXIT_FAILURE);
	}
	fclose(pOutFile2);
	printf("'%s' saved successfully!\n",fileOutB);
	free(outBuf2);

	return EXIT_SUCCESS;
}
/*----------------------------------------------------------------------------*/

/* WordSplit(char *fileIn, char *fileOutA, char *fileOutB) - /w
 *
 * (Params)
 * char *fileIn			Input filename
 * char *fileOutA		Output filename 1
 * char *fileOutB		Output filename 2
 */
int WordSplit(char *fileIn, char *fileOutA, char *fileOutB){
	FILE *pInFile, *pOutFile1, *pOutFile2;
	long length;
	unsigned char *inBuffer;
	size_t result;
	long halfLength;
	unsigned char *outBuf1;
	unsigned char *outBuf2;
	long i = 0;
	long pos1 = 0;
	long pos2 = 2;

	if(!FileExists(fileIn)){
		return EXIT_FAILURE;
	}
	printf("Splitting file '%s' into words, saving to '%s' and '%s'\n",fileIn,fileOutA,fileOutB);

	pInFile = fopen(fileIn,"rb");
	if(pInFile == NULL){
		perror("Error attempting to open input file");
		exit(EXIT_FAILURE);
	}

	/* find file size */
	length = FileSize(pInFile);
	rewind(pInFile);

	/* copy data into buffer */
	inBuffer = (unsigned char*)malloc(length);
	if(inBuffer == NULL){
		printf("Error allocating memory for input file buffer.");
		exit(EXIT_FAILURE);
	}

	result = fread(inBuffer,sizeof(unsigned char),length,pInFile);
	if(result != length){
		perror("Error reading input file");
		exit(EXIT_FAILURE);
	}
	fclose(pInFile);

	/* prepare output buffers */
	halfLength = length/2;
	outBuf1 = (unsigned char*)malloc(halfLength);
	if(outBuf1 == NULL){
		printf("Error allocating memory for first output file buffer.");
		exit(EXIT_FAILURE);
	}
	outBuf2 = (unsigned char*)malloc(halfLength);
	if(outBuf2 == NULL){
		printf("Error allocating memory for second output file buffer.");
		exit(EXIT_FAILURE);
	}

	/* fill buffers */
	while(i<halfLength){
		outBuf1[i] = inBuffer[pos1];
		outBuf1[i+1] = inBuffer[pos1+1];
		outBuf2[i] = inBuffer[pos2];
		outBuf2[i+1] = inBuffer[pos2+1];
		i+=2;
		pos1+=4;
		pos2+=4;
	}
	free(inBuffer);

	/* write output files */
	pOutFile1 = fopen(fileOutA,"wb");
	if(pOutFile1 == NULL){
		perror("Error attempting to create first output file");
		exit(EXIT_FAILURE);
	}
	result = fwrite(outBuf1,sizeof(unsigned char),halfLength,pOutFile1);
	if(result != halfLength){
		perror("Error writing first output file");
		exit(EXIT_FAILURE);
	}
	fclose(pOutFile1);
	printf("'%s' saved successfully!\n",fileOutA);
	free(outBuf1);

	pOutFile2 = fopen(fileOutB,"wb");
	if(pOutFile2 == NULL){
		perror("Error attempting to create second output file");
		exit(EXIT_FAILURE);
	}
	result = fwrite(outBuf2,sizeof(unsigned char),halfLength,pOutFile2);
	if(result != halfLength){
		perror("Error writing second output file");
		exit(EXIT_FAILURE);
	}
	fclose(pOutFile2);
	printf("'%s' saved successfully!\n",fileOutB);
	free(outBuf2);

	return EXIT_SUCCESS;
}
/*----------------------------------------------------------------------------*/

/* FlipByte(char *fileIn, char *fileOut) - /f
 * Flip low/high bytes of a file.
 *
 * (Params)
 * char *fileIn			Input filename
 * char *fileOut		Output filename
 */
int FlipByte(char *fileIn, char *fileOut){
	FILE *pInFile, *pOutFile;
	long length;
	unsigned char *buffer;
	size_t result;
	long i = 0;
	unsigned char b1, b2;

	if(!FileExists(fileIn)){
		return EXIT_FAILURE;
	}
	/* If the second file is not passed in, flip the file in place. */
	if(fileOut == NULL){
		fileOut = fileIn;
	}

	printf("Flipping bytes of '%s', saving to '%s'\n",fileIn,fileOut);

	pInFile = fopen(fileIn,"rb");
	if(pInFile == NULL){
		perror("Error attempting to open input file");
		exit(EXIT_FAILURE);
	}

	/* find file size */
	length = FileSize(pInFile);
	rewind(pInFile);

	/* copy data into buffer */
	buffer = (unsigned char*)malloc(length);
	if(buffer == NULL){
		printf("Error allocating memory for buffer.");
		exit(EXIT_FAILURE);
	}

	result = fread(buffer,sizeof(unsigned char),length,pInFile);
	if(result != length){
		perror("Error reading input file");
		exit(EXIT_FAILURE);
	}
	fclose(pInFile);

	/* Create new file */
	pOutFile = fopen(fileOut,"wb");
	if(pOutFile == NULL){
		perror("Error attempting to create output file");
		exit(EXIT_FAILURE);
	}

	/* flip bytes in buffer */
	while(i<length){
		b1 = buffer[i];
		b2 = buffer[i+1];
		buffer[i+1] = b1;
		buffer[i] = b2;
		i+=2;
	}

	/* write output file */
	result = fwrite(buffer,sizeof(unsigned char),length,pOutFile);
	if(result != length){
		perror("Error writing output file");
		exit(EXIT_FAILURE);
	}
	fclose(pOutFile);
	printf("'%s' saved successfully!\n",fileOut);

	free(buffer);
	return EXIT_SUCCESS;
}
/*----------------------------------------------------------------------------*/

/* MergeBytes(char *fileIn1, char *fileIn2, char *fileOut) - /m
 * Byte merges two files; stores result in fileOut.
 *
 * (Params)
 * char *fileIn1		Input filename 1
 * char *fileIn2		Input filename 2
 * char *fileOut		Output filename
 */
int MergeBytes(char *fileIn1, char *fileIn2, char *fileOut){
	FILE *pInFile1, *pInFile2, *pOutFile;
	long length1, length2;
	unsigned char *inBuf1;
	unsigned char *inBuf2;
	size_t result;
	long outBufLen;
	unsigned char *outBuf;
	long i = 0;
	long curPos = 0;

	if(!FileExists(fileIn1)){
		return EXIT_FAILURE;
	}
	if(!FileExists(fileIn2)){
		return EXIT_FAILURE;
	}

	printf("Merging bytes of '%s' and '%s', saving to '%s'\n",fileIn1,fileIn2,fileOut);

	/* Read file 1 */
	pInFile1 = fopen(fileIn1,"rb");
	if(pInFile1 == NULL){
		perror("Error attempting to open first input file");
		exit(EXIT_FAILURE);
	}

	/* find first file size */
	length1 = FileSize(pInFile1);
	rewind(pInFile1);

	/* put file 1's contents into buffer */
	inBuf1 = (unsigned char*)malloc(length1);
	if(inBuf1 == NULL){
		printf("Error allocating memory for input file buffer 1.");
		exit(EXIT_FAILURE);
	}

	result = fread(inBuf1,sizeof(unsigned char),length1,pInFile1);
	if(result != length1){
		perror("Error reading first input file");
		exit(EXIT_FAILURE);
	}
	fclose(pInFile1);

	/* Read file 2 */
	pInFile2 = fopen(fileIn2,"rb");
	if(pInFile2 == NULL){
		perror("Error attempting to open second input file");
		exit(EXIT_FAILURE);
	}

	/* find second file size */
	length2 = FileSize(pInFile2);
	rewind(pInFile2);

	/* put file 2's contents into buffer */
	inBuf2 = (unsigned char*)malloc(length2);
	if(inBuf2 == NULL){
		printf("Error allocating memory for input file buffer 2.");
		exit(EXIT_FAILURE);
	}

	result = fread(inBuf2,sizeof(unsigned char),length2,pInFile2);
	if(result != length2){
		perror("Error reading second input file");
		exit(EXIT_FAILURE);
	}
	fclose(pInFile2);

	/* Create new file */
	pOutFile = fopen(fileOut,"wb");
	if(pOutFile == NULL){
		perror("Error attempting to create output file");
		exit(EXIT_FAILURE);
	}

	/* merge bytes into a new buffer */
	outBufLen = length1+length2;
	outBuf = (unsigned char*)malloc(outBufLen);
	if(outBuf == NULL){
		printf("Error allocating memory for output file buffer.");
		exit(EXIT_FAILURE);
	}

	while(i<length1){
		outBuf[curPos] = inBuf1[i];
		outBuf[curPos+1] = inBuf2[i];
		curPos+=2;
		i++;
	}
	free(inBuf1);
	free(inBuf2);

	/* write output file */
	result = fwrite(outBuf,sizeof(unsigned char),outBufLen,pOutFile);
	if(result != outBufLen){
		perror("Error writing output file");
		exit(EXIT_FAILURE);
	}
	fclose(pOutFile);
	printf("'%s' saved successfully!\n",fileOut);

	free(outBuf);
	return EXIT_SUCCESS;
}
/*----------------------------------------------------------------------------*/

/* MergeBytesQuad(char *fileIn1, char *fileIn2, char *fileIn3, char *fileIn4, char *fileOut) - /q
 * Byte merges four files; stores result in fileOut.
 *
 * (Params)
 * char *fileIn1		Input filename 1
 * char *fileIn2		Input filename 2
 * char *fileIn3		Input filename 3
 * char *fileIn4		Input filename 4
 * char *fileOut		Output filename
 */
int MergeBytesQuad(char *fileIn1, char *fileIn2, char *fileIn3, char *fileIn4, char *fileOut){
	FILE *pInFile1, *pInFile2, *pInFile3, *pInFile4, *pOutFile;
	long length1, length2, length3, length4;
	unsigned char *inBuf1;
	unsigned char *inBuf2;
	unsigned char *inBuf3;
	unsigned char *inBuf4;
	size_t result;
	long outBufLen;
	unsigned char *outBuf;
	long i = 0;
	long curPos = 0;

	if(!FileExists(fileIn1)){
		return EXIT_FAILURE;
	}
	if(!FileExists(fileIn2)){
		return EXIT_FAILURE;
	}
	if(!FileExists(fileIn3)){
		return EXIT_FAILURE;
	}
	if(!FileExists(fileIn4)){
		return EXIT_FAILURE;
	}

	printf("Merging bytes of '%s', '%s', '%s', and '%s'; saving to '%s'\n",
		fileIn1,fileIn2,fileIn3,fileIn4,fileOut
	);

	/* Read file 1 */
	pInFile1 = fopen(fileIn1,"rb");
	if(pInFile1 == NULL){
		perror("Error attempting to open first input file");
		exit(EXIT_FAILURE);
	}

	/* find first file size */
	length1 = FileSize(pInFile1);
	rewind(pInFile1);

	/* put file 1's contents into buffer */
	inBuf1 = (unsigned char*)malloc(length1);
	if(inBuf1 == NULL){
		printf("Error allocating memory for input file buffer 1.");
		exit(EXIT_FAILURE);
	}

	result = fread(inBuf1,sizeof(unsigned char),length1,pInFile1);
	if(result != length1){
		perror("Error reading first input file");
		exit(EXIT_FAILURE);
	}
	fclose(pInFile1);

	/* Read file 2 */
	pInFile2 = fopen(fileIn2,"rb");
	if(pInFile2 == NULL){
		perror("Error attempting to open second input file");
		exit(EXIT_FAILURE);
	}

	/* find second file size */
	length2 = FileSize(pInFile2);
	rewind(pInFile2);

	/* put file 2's contents into buffer */
	inBuf2 = (unsigned char*)malloc(length2);
	if(inBuf2 == NULL){
		printf("Error allocating memory for input file buffer 2.");
		exit(EXIT_FAILURE);
	}

	result = fread(inBuf2,sizeof(unsigned char),length2,pInFile2);
	if(result != length2){
		perror("Error reading second input file");
		exit(EXIT_FAILURE);
	}
	fclose(pInFile2);

	/* Read file 3 */
	pInFile3 = fopen(fileIn3,"rb");
	if(pInFile3 == NULL){
		perror("Error attempting to open third input file");
		exit(EXIT_FAILURE);
	}

	/* find third file size */
	length3 = FileSize(pInFile3);
	rewind(pInFile3);

	/* put file 3's contents into buffer */
	inBuf3 = (unsigned char*)malloc(length3);
	if(inBuf3 == NULL){
		printf("Error allocating memory for input file buffer 3.");
		exit(EXIT_FAILURE);
	}

	result = fread(inBuf3,sizeof(unsigned char),length3,pInFile3);
	if(result != length3){
		perror("Error reading third input file");
		exit(EXIT_FAILURE);
	}
	fclose(pInFile3);

	/* Read file 4 */
	pInFile4 = fopen(fileIn4,"rb");
	if(pInFile4 == NULL){
		perror("Error attempting to open fourth input file");
		exit(EXIT_FAILURE);
	}

	/* find fourth file size */
	length4 = FileSize(pInFile4);
	rewind(pInFile4);

	/* put file 4's contents into buffer */
	inBuf4 = (unsigned char*)malloc(length4);
	if(inBuf4 == NULL){
		printf("Error allocating memory for input file buffer 4.");
		exit(EXIT_FAILURE);
	}

	result = fread(inBuf4,sizeof(unsigned char),length4,pInFile4);
	if(result != length4){
		perror("Error reading fourth input file");
		exit(EXIT_FAILURE);
	}
	fclose(pInFile4);

	/* Create new file */
	pOutFile = fopen(fileOut,"wb");
	if(pOutFile == NULL){
		perror("Error attempting to create output file");
		exit(EXIT_FAILURE);
	}

	/* merge bytes into a new buffer */
	outBufLen = length1+length2+length3+length4;
	outBuf = (unsigned char*)malloc(outBufLen);
	if(outBuf == NULL){
		printf("Error allocating memory for output file buffer.");
		exit(EXIT_FAILURE);
	}

	while(i<length1){
		outBuf[curPos] = inBuf1[i];
		outBuf[curPos+1] = inBuf2[i];
		outBuf[curPos+2] = inBuf3[i];
		outBuf[curPos+3] = inBuf4[i];
		curPos+=4;
		i++;
	}
	free(inBuf1);
	free(inBuf2);
	free(inBuf3);
	free(inBuf4);

	/* write output file */
	result = fwrite(outBuf,sizeof(unsigned char),outBufLen,pOutFile);
	if(result != outBufLen){
		perror("Error writing output file");
		exit(EXIT_FAILURE);
	}
	fclose(pOutFile);
	printf("'%s' saved successfully!\n",fileOut);

	free(outBuf);
	return EXIT_SUCCESS;
}
/*----------------------------------------------------------------------------*/

/* UpdateBytes(char *fileIn1, char *fileIn2,size_t offset, char *fileOut) - /m
 * Byte update two files; stores result in fileOut.
 * Content of fileIn1 overwrite content of fileIn2 with offset
 *
 * (Params)
 * char *fileIn1		Input filename 1
 * char *fileIn2		Input filename 2
 * char *fileOut		Output filename
 * size_t offset		Update size

 */
int UpdateBytes(char *fileIn1, char *fileIn2,char *fileOut,char* updateSize) {
	FILE *pInFile1, *pInFile2, *pOutFile;
	long length1, length2;
	unsigned char *inBuf1;
	unsigned char *inBuf2;
	size_t result;
	long outBufLen;
	/*unsigned char *outBuf;*/
	long i = 0;
	long curPos = 0;
	size_t size;

	if (updateSize != NULL) {
		size = atol(updateSize);
	}
	else {
		perror("Error need size parameter");
		exit(EXIT_FAILURE);
	}

	if (!FileExists(fileIn1)) {
		return EXIT_FAILURE;
	}
	if (!FileExists(fileIn2)) {
		return EXIT_FAILURE;
	}

	printf("Updating (%u)bytes of '%s' to '%s, saving to '%s'\n",size, fileIn1, fileIn2, fileOut);

	/* Read file 1 */
	pInFile1 = fopen(fileIn1, "rb");
	if (pInFile1 == NULL) {
		perror("Error attempting to open first input file");
		exit(EXIT_FAILURE);
	}

	/* find first file size */
	length1 = FileSize(pInFile1);
	rewind(pInFile1);

	if (size > length1) {
		printf("Error update size larger than file buffer 1.");
		exit(EXIT_FAILURE);
	}

	/* put file 1's contents into buffer */
	inBuf1 = (unsigned char*)malloc(length1);
	if (inBuf1 == NULL) {
		printf("Error allocating memory for input file buffer 1.");
		exit(EXIT_FAILURE);
	}

	result = fread(inBuf1, sizeof(unsigned char), length1, pInFile1);
	if (result != length1) {
		perror("Error reading first input file");
		exit(EXIT_FAILURE);
	}
	fclose(pInFile1);

	/* Read file 2 */
	pInFile2 = fopen(fileIn2, "rb");
	if (pInFile2 == NULL) {
		perror("Error attempting to open second input file");
		exit(EXIT_FAILURE);
	}

	/* find second file size */
	length2 = FileSize(pInFile2);
	/*
	if ((offset + length1) > length2) {
		printf("Error update overflow file buffer1 size and/or offset are out bounds.");
		exit(EXIT_FAILURE);
	}
	*/

	rewind(pInFile2);

	/* put file 2's contents into buffer */
	inBuf2 = (unsigned char*)malloc(length2);
	if (inBuf2 == NULL) {
		printf("Error allocating memory for input file buffer 2.");
		exit(EXIT_FAILURE);
	}

	result = fread(inBuf2, sizeof(unsigned char), length2, pInFile2);
	if (result != length2) {
		perror("Error reading second input file");
		exit(EXIT_FAILURE);
	}
	fclose(pInFile2);

	memcpy(inBuf2, inBuf1, size);

	/* Create new file */
	pOutFile = fopen(fileOut, "wb");
	if (pOutFile == NULL) {
		perror("Error attempting to create output file");
		exit(EXIT_FAILURE);
	}

	/* write output file */
	result = fwrite(inBuf2, sizeof(unsigned char), length2, pOutFile);
	if (result != length2) {
		perror("Error writing output file");
		exit(EXIT_FAILURE);
	}

	free(inBuf1);
	free(inBuf2);

	fclose(pOutFile);
	printf("'%s' saved successfully!\n", fileOut);

	/*free(outBuf);*/
	return EXIT_SUCCESS;
}

/*----------------------------------------------------------------------------*/

/* SwapHalf(char *fileIn, char *fileOut) - /s
 * Swaps the top and bottom halves of fileIn; writes to fileOut.
 *
 * (Params)
 * char *fileIn			Input filename
 * char *fileOut		Output filename
 */
int SwapHalf(char *fileIn, char *fileOut){
	FILE *pInFile, *pOutFile;
	long length, halfLength;
	unsigned char *inBufHalf1;
	unsigned char *inBufHalf2;
	size_t result;

	if(!FileExists(fileIn)){
		return EXIT_FAILURE;
	}
	/* If the second file is not passed in, swap the file in place. */
	if(fileOut == NULL){
		fileOut = fileIn;
	}

	printf("Swapping halves of '%s', saving to '%s'\n",fileIn,fileOut);

	pInFile = fopen(fileIn,"rb");
	if(pInFile == NULL){
		perror("Error attempting to open input file");
		exit(EXIT_FAILURE);
	}

	/* find current file size */
	length = FileSize(pInFile);
	rewind(pInFile);
	halfLength = length/2;

	/* read halves of file into two buffers */
	inBufHalf1 = (unsigned char*)malloc(halfLength);
	if(inBufHalf1 == NULL){
		printf("Error allocating memory for input file buffer 1.");
		exit(EXIT_FAILURE);
	}
	result = fread(inBufHalf1,sizeof(unsigned char),halfLength,pInFile);
	if(result != halfLength){
		perror("Error reading input file");
		exit(EXIT_FAILURE);
	}

	inBufHalf2 = (unsigned char*)malloc(halfLength);
	if(inBufHalf2 == NULL){
		printf("Error allocating memory for input file buffer 1.");
		exit(EXIT_FAILURE);
	}
	result = fread(inBufHalf2,sizeof(unsigned char),halfLength,pInFile);
	if(result != halfLength){
		perror("Error reading input file");
		exit(EXIT_FAILURE);
	}

	fclose(pInFile);

	/* create new file from buffers written in reverse order */
	pOutFile = fopen(fileOut,"wb");
	if(pOutFile == NULL){
		perror("Error attempting to create output file");
		exit(EXIT_FAILURE);
	}

	/* write output file */
	result = fwrite(inBufHalf2,sizeof(unsigned char),halfLength,pOutFile);
	if(result != halfLength){
		perror("Error writing output file");
		exit(EXIT_FAILURE);
	}
	result = fwrite(inBufHalf1,sizeof(unsigned char),halfLength,pOutFile);
	if(result != halfLength){
		perror("Error writing output file");
		exit(EXIT_FAILURE);
	}
	fclose(pOutFile);
	printf("'%s' saved successfully!\n",fileOut);

	free(inBufHalf1);
	free(inBufHalf2);
	return EXIT_SUCCESS;
}
/*----------------------------------------------------------------------------*/

/* PadFile(char *fileIn, char *fileOut, char *padSize, char *padByte) - /p
 * Pads fileIn to padSize kilobytes with specified padByte; writes to fileOut.
 *
 * (Params)
 * char *fileIn			Input filename
 * char *fileOut		Output filename
 * char *padSize		Size to pad file to (in Kilobytes)
 * char *padByte		Value to pad with
 */
int PadFile(char *fileIn, char *fileOut, char *padSize, char *padByte){
	unsigned int shortPadSize = (atoi(padSize));
	unsigned char padChar = (unsigned char)atoi(padByte);
	FILE *pInFile, *pOutFile;
	long length, fullPadSize,bufLength;
	unsigned char *buffer;
	size_t result;
	long remain, bufPos;
	long i = 0;

	if(!FileExists(fileIn)){
		return EXIT_FAILURE;
	}

	printf("Padding '%s' to %d kilobytes with byte 0x%02X, saving to '%s'\n",
		fileIn,shortPadSize,padChar,fileOut);

	pInFile = fopen(fileIn,"rb");
	if(pInFile == NULL){
		perror("Error attempting to open input file");
		exit(EXIT_FAILURE);
	}

	/* find current file size */
	length = FileSize(pInFile);
	rewind(pInFile);

	/* copy data into buffer */
	fullPadSize = shortPadSize*1024;
	bufLength = fullPadSize;
	buffer = (unsigned char*)malloc(bufLength);
	if(buffer == NULL){
		printf("Error allocating memory for input file buffer.");
		exit(EXIT_FAILURE);
	}

	result = fread(buffer,sizeof(unsigned char),length,pInFile);
	if(result != length){
		perror("Error reading input file");
		exit(EXIT_FAILURE);
	}

	fclose(pInFile);

	/* add padding to buffer */
	remain = fullPadSize-(sizeof(unsigned char)*length);
	bufPos = result;
	while(i<remain){
		buffer[bufPos] = padChar;
		bufPos++;
		i++;
	}

	/* create new file with padding */
	pOutFile = fopen(fileOut,"wb");
	if(pOutFile == NULL){
		perror("Error attempting to create output file");
		exit(EXIT_FAILURE);
	}

	/* write output file */
	result = fwrite(buffer,sizeof(unsigned char),bufLength,pOutFile);
	if(result != bufLength){
		perror("Error writing output file");
		exit(EXIT_FAILURE);
	}
	fclose(pOutFile);
	printf("'%s' saved successfully!\n",fileOut);

	free(buffer);
	return EXIT_SUCCESS;
}
/*----------------------------------------------------------------------------*/

/* ConcatFiles(char *fileIn, char *fileOutA, char *fileOutB) - /b
 *
 * (Params)
 * char *fileInA			Input filename 1
 * char *fileInB			Input filename 2
 * char *fileOut			Output filename 
 *
 * OzzyOuzo   note: bored to mess up with system commands thus better to get it included into romwak
 *			  pretty useful to generate appropriate P roms when using bank switching.
 */

 /* #define USE_PRINTF_ERRORS */
int ConcatFiles(char *fileInA_, char *fileInB_, char *fileOut_)
{
	FILE *pInFileA, *pInFileB, *pOutFile;
	long sizeA,sizeB;
	unsigned char *inBufA,*inBufB;
	size_t result;

	if (!FileExists(fileInA_) || !FileExists(fileInB_)) {
		#ifdef USE_PRINTF_ERRORS
		printf("Error input file not found\n");
		#endif
		return EXIT_FAILURE;
	}


	/* file A */

	pInFileA = fopen(fileInA_, "rb");
	if (pInFileA == NULL) {
		#ifdef USE_PRINTF_ERRORS
		printf("Error attempting to open input file A\n");
		#endif
		perror("Error attempting to open input file A");
		exit(EXIT_FAILURE);
	}

	sizeA = FileSize(pInFileA);
	fseek(pInFileA, 0, SEEK_SET);

	if (!sizeA) {
		#ifdef USE_PRINTF_ERRORS
		printf("Error empty file A\n");
		#endif
		perror("Error empty file A");
		exit(EXIT_FAILURE);
	}

	inBufA = (unsigned char*)malloc(sizeA);
	if (inBufA == NULL){
		#ifdef USE_PRINTF_ERRORS
		printf("Error allocating memory for input file A buffer.\n");
		#endif
		perror("Error allocating memory for input file A buffer.");
		exit(EXIT_FAILURE);
	}

	result = fread(inBufA, sizeof(unsigned char),sizeA, pInFileA);
	if (result != sizeA){
		#ifdef USE_PRINTF_ERRORS
		printf("Error reading input file A\n");
		#endif
		perror("Error reading input file A");
		exit(EXIT_FAILURE);
	}
	fclose(pInFileA);

	/* file B */

	pInFileB = fopen(fileInB_, "rb");
	if (pInFileB == NULL) {
		#ifdef USE_PRINTF_ERRORS
		printf("Error attempting to open input file B\n");
		#endif
		perror("Error attempting to open input file B");
		exit(EXIT_FAILURE);
	}

	sizeB = FileSize(pInFileB);
	fseek(pInFileB, 0, SEEK_SET);

	if (!sizeB) {
		#ifdef USE_PRINTF_ERRORS
		printf("Error empty file B\n");
		#endif
		perror("Error empty file B");
		exit(EXIT_FAILURE);
	}

	inBufB = (unsigned char*)malloc(sizeB);
	if (inBufB == NULL) {
		#ifdef USE_PRINTF_ERRORS	
		printf("Error allocating memory for input file B buffer.\n");
		#endif
		perror("Error allocating memory for input file B buffer.");
		exit(EXIT_FAILURE);
	}
	
	result = fread(inBufB, sizeof(unsigned char), sizeB, pInFileB);
	if (result != sizeB) {
		#ifdef USE_PRINTF_ERRORS	
		printf("Error reading input file B\n");
		#endif
		perror("Error reading input file B");
		exit(EXIT_FAILURE);
	}
	fclose(pInFileB);

	/* create concatened file */
	pOutFile = fopen(fileOut_, "wb");
	if (pOutFile == NULL) {
		#ifdef USE_PRINTF_ERRORS	
		printf("Error attempting to create output file\n");
		#endif
		perror("Error attempting to create output file");
		exit(EXIT_FAILURE);
	}

	result = fwrite(inBufA, sizeof(unsigned char), sizeA, pOutFile);
	if (result != sizeA) {
		#ifdef USE_PRINTF_ERRORS	
		printf("Error writing part A of output file\n");
		#endif
		perror("Error writing part A of output file");
		exit(EXIT_FAILURE);
	}

	result = fwrite(inBufB, sizeof(unsigned char), sizeB, pOutFile);
	if (result != sizeB) {
		#ifdef USE_PRINTF_ERRORS	
		printf("Error writing part B of output file\n");
		#endif
		perror("Error writing part B of output file");
		exit(EXIT_FAILURE);
	}

	fclose(pOutFile);

	printf("'%s' + '%s' concatained into '%s' successfully!\n", fileInA_, fileInB_, fileOut_);

	free(inBufA);
	free(inBufB);
	return EXIT_SUCCESS;
}

/* ConcatFilesEx(char *fileIn, char *fileOutA, char *fileOutB) - /b
 *
 * (Params)
 * char *fileInA			Input filename 1
 * char *fileInB			Input filename 2
 * char *pathOut			Output path for generated prom and prom1 files.
 *
 * OzzyOuzo   note: Darksoft special concatenation for proms files.
 *
 */

 /*
 example for teot:

copy teot-m1.bin p:\foo\darksoft\m1rom
copy teot-s1.bin p:\foo\darksoft\srom
romwak /e teot-p1.bin teot-p2.bin p:\foo\darksoft
romwak /c teot-v1.bin teot-v2.bin p:\foo\darksoft\vroma0
romwak /d teot-c1.bin teot-c2.bin p:\foo\darksoft\crom0

 as it is using a MiSter core format you'll need to add a text file containing 2 to 3 digits called:fpga
 into the generated roms directory.

 1st digit indicates bankswitching mode:
 ---------------------------------------
 0 = no bankswitching
 1 = standard bankswitching
 2 = neo-pvc bankswitching (F9 & F8)
 3 = neo-sma KOF99 bankswitching
 4 = neo-sma Garou "KF" version bankswitching
 5 = neo-sma Garou "KE" version bankswitching
 6 = neo-sma Metal Slug 3 bankswitching
 7 = neo-sma KOF2000 bankswitching

 2nd digit indicates graphics mode:
 ----------------------------------
 0 = CROM full address
 1 = CROM zero 1 MSB
 2 = CROM zero 2 MSBs
 3 = CROM zero 3 MSBs
 4 = CROM zero 4 MSBs
 5 = NEO-CMC SROM bankswitching (42 version)
 6 = NEO-CMC SROM bankswitching (50 version)

 3rd digit indicates audio mode:
 -------------------------------
 0 = VROM+PCM mode
 1 = VROMA/VROMB mode

 generally the value:10 is working fine.



 /* #define USE_PRINTF_ERRORS */
int ConcatFilesEx(char* fileInA_, char* fileInB_, char* pathOut_)
{
	FILE* pInFileA, * pInFileB, * pOutFile;
	long sizeA, sizeB, sizeC, finalSize;
	unsigned char* inBufA, * inBufB;
	size_t result;
	char fileOut[8192];

	if (!FileExists(fileInA_) || !FileExists(fileInB_)) {
		#ifdef USE_PRINTF_ERRORS
		printf("Error input file not found\n");
		#endif
		return EXIT_FAILURE;
	}


	/* file A */

	pInFileA = fopen(fileInA_, "rb");
	if (pInFileA == NULL) {
		#ifdef USE_PRINTF_ERRORS
		printf("Error attempting to open input file A\n");
		#endif
		perror("Error attempting to open input file A");
		exit(EXIT_FAILURE);
	}

	sizeA = FileSize(pInFileA);
	fseek(pInFileA, 0, SEEK_SET);

	if (!sizeA) {
		#ifdef USE_PRINTF_ERRORS
		printf("Error empty file A\n");
		#endif
		perror("Error empty file A");
		exit(EXIT_FAILURE);
	}

	inBufA = (unsigned char*)malloc(sizeA);
	if (inBufA == NULL) {
		#ifdef USE_PRINTF_ERRORS
		printf("Error allocating memory for input file A buffer.\n");
		#endif
		perror("Error allocating memory for input file A buffer.");
		exit(EXIT_FAILURE);
	}

	result = fread(inBufA, sizeof(unsigned char), sizeA, pInFileA);
	if (result != sizeA) {
		#ifdef USE_PRINTF_ERRORS
		printf("Error reading input file A\n");
		#endif
		perror("Error reading input file A");
		exit(EXIT_FAILURE);
	}
	fclose(pInFileA);

	/* file B */

	pInFileB = fopen(fileInB_, "rb");
	if (pInFileB == NULL) {
		#ifdef USE_PRINTF_ERRORS
		printf("Error attempting to open input file B\n");
		#endif
		perror("Error attempting to open input file B");
		exit(EXIT_FAILURE);
	}

	sizeB = FileSize(pInFileB);
	fseek(pInFileB, 0, SEEK_SET);

	if (!sizeB) {
		#ifdef USE_PRINTF_ERRORS
		printf("Error empty file B\n");
		#endif
		perror("Error empty file B");
		exit(EXIT_FAILURE);
	}

	inBufB = (unsigned char*)malloc(sizeB);
	if (inBufB == NULL) {
		#ifdef USE_PRINTF_ERRORS	
		printf("Error allocating memory for input file B buffer.\n");
		#endif
		perror("Error allocating memory for input file B buffer.");
		exit(EXIT_FAILURE);
	}

	result = fread(inBufB, sizeof(unsigned char), sizeB, pInFileB);
	if (result != sizeB) {
		#ifdef USE_PRINTF_ERRORS	
		printf("Error reading input file B\n");
		#endif
		perror("Error reading input file B");
		exit(EXIT_FAILURE);
	}
	fclose(pInFileB);

	/* create concatened files */

	sprintf(fileOut,"%s/prom",pathOut_);

	pOutFile = fopen(fileOut, "wb");
	if (pOutFile == NULL) {
		#ifdef USE_PRINTF_ERRORS	
		printf("Error attempting to create prom file\n");
		#endif
		perror("Error attempting to create prom file");
		exit(EXIT_FAILURE);
	}

	if (sizeA > EIGHT_MB) {
		finalSize = EIGHT_MB;
	}
	else {
		finalSize = sizeA;
	}

	result = fwrite(inBufA, sizeof(unsigned char), finalSize, pOutFile);
	if (result != finalSize) {
		#ifdef USE_PRINTF_ERRORS	
		printf("Error writing part A of prom file\n");
		#endif
		perror("Error writing part A of prom file");
		exit(EXIT_FAILURE);
	}

	if (sizeA > EIGHT_MB) {
		fclose(pOutFile);

		sprintf(fileOut, "%s/prom1", pathOut_);

		pOutFile = fopen(fileOut, "wb");
		if (pOutFile == NULL) {
			#ifdef USE_PRINTF_ERRORS	
			printf("Error attempting to create prom1 file\n");
			#endif
			perror("Error attempting to create prom1 file");
			exit(EXIT_FAILURE);
		}

		finalSize = sizeA - finalSize;

		result = fwrite(inBufA, sizeof(unsigned char), finalSize, pOutFile);
		if (result != finalSize) {
			#ifdef USE_PRINTF_ERRORS	
			printf("Error writing part A of prom1 file\n");
			#endif
			perror("Error writing part A of prom1 file");
			exit(EXIT_FAILURE);
		}
	}
	else {
		sizeC = sizeA + sizeB;
		if (sizeC > EIGHT_MB) {
			finalSize = sizeC - EIGHT_MB;
			sizeB -= finalSize;
		}
	}

	result = fwrite(inBufB, sizeof(unsigned char), sizeB, pOutFile);
	if (result != sizeB) {
		if (sizeA > EIGHT_MB) {
			#ifdef USE_PRINTF_ERRORS	
			printf("Error writing part B of prom1 file\n");
			#endif
			perror("Error writing part B of prom1 file");
		}
		else {
			#ifdef USE_PRINTF_ERRORS	
			printf("Error writing part B of prom file\n");
			#endif
			perror("Error writing part B of prom file");
		}
		exit(EXIT_FAILURE);
	

	}

	if (sizeC > EIGHT_MB && sizeA <= EIGHT_MB) {
		fclose(pOutFile);

		sprintf(fileOut, "%s/prom1", pathOut_);

		pOutFile = fopen(fileOut, "wb");
		if (pOutFile == NULL) {
			#ifdef USE_PRINTF_ERRORS	
			printf("Error attempting to create prom1 file\n");
			#endif
			perror("Error attempting to create prom1 file");
			exit(EXIT_FAILURE);
		}

		result = fwrite(&inBufB[sizeB], sizeof(unsigned char), finalSize, pOutFile);
		if (result != finalSize) {
			#ifdef USE_PRINTF_ERRORS	
			printf("Error writing prom1 file\n");
			#endif
			perror("Error writing prom1 file");
			exit(EXIT_FAILURE);
		}
	}

	fclose(pOutFile);

	printf("'%s' + '%s' concatained into prom ",fileInA_,fileInB_);
	if (sizeC > EIGHT_MB || sizeA > EIGHT_MB) {
		printf("and prom1 ");
	}
		
	printf("successfully!\n");

	free(inBufA);
	free(inBufB);
	return EXIT_SUCCESS;
}


/* DarksoftConcatFiles(char *fileIn, char *fileOutA, char *fileOutB) - /b
 *
 * (Params)
 * char *fileInA			Input filename 1
 * char *fileInB			Input filename 2
 * char *fileOut			Output filename
 *
 * OzzyOuzo   note: Darksoft special word concatenation for croms files.
 *
 */

 /*
 example for cyborgForce:

 copy cyborg-m1.bin p:\foo\Darksoft\cyborgf\m1rom
 copy cyborg-s1.bin p:\foo\Darksoft\cyborgf\srom
 romwak /c cyborg-p1.bin cyborg-p2.bin p:\foo\Darksoft\cyborgf\prom
 romwak /c cyborg-v1.bin cyborg-v2.bin p:\foo\Darksoft\cyborgf\vroma0
 romwak /d cyborg-c1.bin cyborg-c2.bin p:\foo\Darksoft\cyborgf\crom0

 as it is using a MiSter core format you'll need to add a text file containing 2 to 3 digits called:fpga
 into the generated roms directory.

 1st digit indicates bankswitching mode:
 ---------------------------------------
 0 = no bankswitching
 1 = standard bankswitching
 2 = neo-pvc bankswitching (F9 & F8)
 3 = neo-sma KOF99 bankswitching
 4 = neo-sma Garou "KF" version bankswitching
 5 = neo-sma Garou "KE" version bankswitching
 6 = neo-sma Metal Slug 3 bankswitching
 7 = neo-sma KOF2000 bankswitching

 2nd digit indicates graphics mode:
 ----------------------------------
 0 = CROM full address
 1 = CROM zero 1 MSB
 2 = CROM zero 2 MSBs
 3 = CROM zero 3 MSBs
 4 = CROM zero 4 MSBs
 5 = NEO-CMC SROM bankswitching (42 version)
 6 = NEO-CMC SROM bankswitching (50 version)

 3rd digit indicates audio mode:
 -------------------------------
 0 = VROM+PCM mode
 1 = VROMA/VROMB mode

 generally the value:10 is working fine.

 */

 /* #define USE_PRINTF_ERRORS */
int DarksoftConcatFiles(char* fileInA_, char* fileInB_, char* fileOut_)
{
	FILE* pInFileA, * pInFileB, * pOutFile;
	long i, sizeA, sizeB, sizeC;
	unsigned char* inBufA, * inBufB, * inBufC;
	size_t result;
	unsigned short* ptrA, * ptrB, * ptrC;

	if (!FileExists(fileInA_) || !FileExists(fileInB_)) {
		#ifdef USE_PRINTF_ERRORS
		printf("Error input file not found\n");
		#endif
		return EXIT_FAILURE;
	}


	/* file A */

	pInFileA = fopen(fileInA_, "rb");
	if (pInFileA == NULL) {
		#ifdef USE_PRINTF_ERRORS
		printf("Error attempting to open input file A\n");
		#endif
		perror("Error attempting to open input file A");
		exit(EXIT_FAILURE);
	}

	sizeA = FileSize(pInFileA);
	fseek(pInFileA, 0, SEEK_SET);

	if (!sizeA) {
		#ifdef USE_PRINTF_ERRORS
		printf("Error empty file A\n");
		#endif
		perror("Error empty file A");
		exit(EXIT_FAILURE);
	}

	inBufA = (unsigned char*)malloc(sizeA);
	if (inBufA == NULL) {
		#ifdef USE_PRINTF_ERRORS
		printf("Error allocating memory for input file A buffer.\n");
		#endif
		perror("Error allocating memory for input file A buffer.");
		exit(EXIT_FAILURE);
	}

	result = fread(inBufA, sizeof(unsigned char), sizeA, pInFileA);
	if (result != sizeA) {
		#ifdef USE_PRINTF_ERRORS
		printf("Error reading input file A\n");
		#endif
		perror("Error reading input file A");
		exit(EXIT_FAILURE);
	}
	fclose(pInFileA);

	/* file B */

	pInFileB = fopen(fileInB_, "rb");
	if (pInFileB == NULL) {
		#ifdef USE_PRINTF_ERRORS
		printf("Error attempting to open input file B\n");
		#endif
		perror("Error attempting to open input file B");
		exit(EXIT_FAILURE);
	}

	sizeB = FileSize(pInFileB);
	fseek(pInFileB, 0, SEEK_SET);

	if (!sizeB) {
		#ifdef USE_PRINTF_ERRORS
		printf("Error empty file B\n");
		#endif
		perror("Error empty file B");
		exit(EXIT_FAILURE);
	}

	inBufB = (unsigned char*)malloc(sizeB);
	if (inBufB == NULL) {
		#ifdef USE_PRINTF_ERRORS	
		printf("Error allocating memory for input file B buffer.\n");
		#endif
		perror("Error allocating memory for input file B buffer.");
		exit(EXIT_FAILURE);
	}

	result = fread(inBufB, sizeof(unsigned char), sizeB, pInFileB);
	if (result != sizeB) {
		#ifdef USE_PRINTF_ERRORS	
		printf("Error reading input file B\n");
		#endif
		perror("Error reading input file B");
		exit(EXIT_FAILURE);
	}
	fclose(pInFileB);

	sizeC = sizeA + sizeB;

	inBufC = (unsigned char*)malloc(sizeC);
	if (inBufC == NULL) {
		#ifdef USE_PRINTF_ERRORS	
		printf("Error allocating memory for input file C buffer.\n");
		#endif
		perror("Error allocating memory for input file C buffer.");
		exit(EXIT_FAILURE);
	}

	ptrA = (unsigned short*)inBufA;
	ptrB = (unsigned short*)inBufB;
	ptrC = (unsigned short*)inBufC;

	for (i = 0; i < sizeC; i += 4) {
		*ptrC++ = *ptrA++;
		*ptrC++ = *ptrB++;
	}

	/* create concatened file */
	pOutFile = fopen(fileOut_, "wb");
	if (pOutFile == NULL) {
		#ifdef USE_PRINTF_ERRORS	
		printf("Error attempting to create output file\n");
		#endif
		perror("Error attempting to create output file");
		exit(EXIT_FAILURE);
	}

	result = fwrite(inBufC, sizeof(unsigned char), sizeC, pOutFile);
	if (result != sizeC) {
		#ifdef USE_PRINTF_ERRORS	
		printf("Error writing output file\n");
		#endif
		perror("Error writing output file");
		exit(EXIT_FAILURE);
	}

	fclose(pOutFile);

	printf("'%s' + '%s' darksoft concataination into '%s' successfully!\n", fileInA_, fileInB_, fileOut_);

	free(inBufA);
	free(inBufB);
	free(inBufC);
	return EXIT_SUCCESS;
}


/*----------------------------------------------------------------------------*/

typedef unsigned short CRC16;
typedef unsigned long CRC32;

#define POLYNOMIAL 0x04c11db7L

static unsigned long crc_table[256];

/* generate the table of CRC remainders for all possible bytes */
void gen_crc_table(void)
{
	register unsigned long	crc_accum;
	register int			i, j;

	for (i = 0; i < 256; i++) {
		crc_accum = ((unsigned long)i << 24);
		for (j = 0; j < 8; j++) {
			if (crc_accum & 0x80000000L)
				crc_accum = (crc_accum << 1) ^ POLYNOMIAL;
			else
				crc_accum = (crc_accum << 1);
		}
		crc_table[i] = crc_accum;
	}
	//	return;
}

/* update the CRC on the data block one byte at a time */
CRC32 update_crc(unsigned long crc_accum, char * data_blk_ptr, int data_blk_size)
{
	register int	i, j;

	for (j = 0; j < data_blk_size; j++) {
		i = ((int)(crc_accum >> 24) ^ *data_blk_ptr++) & 0xff;
		crc_accum = (crc_accum << 8) ^ crc_table[i];
	}

	return crc_accum;
}

// ....
void crc32Init(void)
{
	gen_crc_table();
}

// ....
CRC32 crc32GenerateKey(unsigned long crc_accum, char * p_data, int data_size)
{
	return update_crc(crc_accum, p_data, data_size);
}

/*----------------------------------------------------------------------------*/


/* SwapHalf(char *fileIn, char *fileOut) - /s
 * Swaps the top and bottom halves of fileIn; writes to fileOut.
 *
 * (Params)
 * char *fileIn			Input filename
 * char *fileOut		Output filename (text)
 */
int InfoFile(char *fileIn, char *fileOut) {
	FILE *pInFile, *pOutFile;
	long length;
	CRC32 crc;
	unsigned char *inBuf;
	size_t result,nb;

	if (!FileExists(fileIn)) {
		return EXIT_FAILURE;
	}
	printf("Generating file informations of '%s', saving to '%s'\n", fileIn, fileOut);

	pInFile = fopen(fileIn, "rb");
	if (pInFile == NULL) {
		perror("Error attempting to open input file");
		exit(EXIT_FAILURE);
	}

	/* find current file size */
	length = FileSize(pInFile);
	rewind(pInFile);

	/* read halves of file into two buffers */
	inBuf = (unsigned char*)malloc(length);

	if (inBuf == NULL) {
		printf("Error allocating memory for input file buffer.");
		exit(EXIT_FAILURE);
	}

	nb = fread(inBuf,1,length, pInFile);
	if (nb != length){
		perror("Error reading input file");
		exit(EXIT_FAILURE);
	}

	fclose(pInFile);

	crc32Init();

	crc = crc32GenerateKey(0,inBuf,length);
	

	/* create new text file containing rom size and crc informations */
	pOutFile = fopen(fileOut, "wt");
	if (pOutFile == NULL) {
		perror("Error attempting to create output file");
		exit(EXIT_FAILURE);
	}

	fprintf(pOutFile, "%s size:%u crc32:0x%x", fileIn, length, crc);
	printf("%s size:%u , crc:0x%x", fileIn, length, crc);

	fclose(pOutFile);
	printf("'%s' saved successfully!\n", fileOut);

	free(inBuf);
	return EXIT_SUCCESS;
}

/*----------------------------------------------------------------------------*/

/* ye olde main */
int main(int argc, char* argv[]){
	printf("ROMWak %s - original version by Jeff Kurtz / ANSI C port by freem / additions from ozzyouzo -\n",ROMWAK_VERSION);
	if(argc < 2){
		Usage();
		return EXIT_FAILURE; /* failure to run due to no options */
	}

	/* command line argument parsing (originally in ROMWAK.DPR) */
	/* The original program used /switches, but this port allows shorthand
	 * switches with a '-' as well, for people who aren't on Windows. */
	if(argv[1][0] != '/' && argv[1][0] != '-'){
		printf("ERROR: Invalid command %s\n\n",argv[1]);
		Usage();
		return EXIT_FAILURE; /* command syntax is wrong, broheim */
	}
	else{
		switch(argv[1][1]){
			case 'b': /* split file in two, alternating bytes */
				return ByteSplit(argv[2],argv[3],argv[4]);

			case 'c': /* concatenate two file2 */
				return ConcatFiles(argv[2], argv[3], argv[4]);

			case 'd': /* concatenate crom files ala Darksoft */
				return DarksoftConcatFiles(argv[2], argv[3], argv[4]);

			case 'e': /* concatenate prom files ala Darksoft */
				return ConcatFilesEx(argv[2], argv[3], argv[4]);

			case 'f': /* flip low/high bytes */
				return FlipByte(argv[2],argv[3]);

			case 'h': /* split file in half (two files) */
				return EqualSplit(argv[2],argv[3],argv[4]);

			case 'i': /* rom information (size,crc) */
				return InfoFile(argv[2], argv[3]);

			case 'm': /* byte merge two files */
				return MergeBytes(argv[2],argv[3],argv[4]);

			case 'q': /* byte merge four files */
				return MergeBytesQuad(argv[2],argv[3],argv[4],argv[5],argv[6]);

			case 's': /* swap top and bottom halves of a file */
				return SwapHalf(argv[2],argv[3]);

			case 'u': /* byte update two files with size */
				return UpdateBytes(argv[2], argv[3], argv[4], argv[5]);

			case 'w': /* split file in two, alternating words */
				return WordSplit(argv[2],argv[3],argv[4]);

			case 'p': /* pad file */
				return PadFile(argv[2],argv[3],argv[4],argv[5]);

			default:
				/* option does not exist */
				printf("ERROR: Option '/%c' doesn't exist.\n",argv[1][1]);
				return EXIT_FAILURE;
		}
	}

	/* should not get here, but if it does, let's not make a fuss. */
	return EXIT_SUCCESS;
}
