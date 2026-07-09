#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int checkFormat(FILE *);

typedef struct {
	int cap;
	unsigned char *bbuf;
	int bidx, idx;
} byteBuf;

typedef struct {
	int curInd, curBind;
	byteBuf *bbuf;
} bufReader;

typedef struct n_t {
	char ch;
	struct n_t *left, *right;
} node;

int readFileToBuf(byteBuf *, FILE *, int);
void initBuf(byteBuf *, int);
void freeBuf(byteBuf *);
void dispBufContents(byteBuf *);

void initReader(bufReader *, byteBuf *);
unsigned char nextBit(bufReader *);
unsigned char nextByte(bufReader *);
void advanceReader(bufReader *); 
void dispReader(bufReader *);

void makeTree(node **, bufReader *);
void dispNode(node *nd);
void huffmanDecode(node *, bufReader *, int, FILE *);
void freeNode(node *);

int main(int argc, char **argv) {
	if (argc != 3) {
		printf("Usage: %s input_filename output_filename\n", argv[0]);
		return 1;
	}
	FILE *fp = fopen(argv[1], "r");
	if (!fp) {
		printf("File %s can't be found\n", argv[1]);
		return 1;
	}

	if (checkFormat(fp)) {
		printf("File is either corrupted or an invalid huffman encoded file\n");
		fclose(fp);
		return 1;
	}

	printf("Format looks alright, moving on...");

/*	char format[5];
	fread(&format, sizeof(char), 3, fp);
	format[3] = '\0';
	printf("Format: %s\n", format);

	if (strncmp(format, "HUF", 3)) {
		printf("Inappropriate format\n");
		return 1;
	} */

	byteBuf bb;
	initBuf(&bb, 5);

	int nbits = 0;
	int read_bytes = fread(&nbits, sizeof(int), 1, fp);
	printf("read %d bytes, nbits = %d\n", read_bytes, nbits);

	int sts = readFileToBuf(&bb, fp, nbits);
	if (sts) {
		printf("Unfinished file\n");
		freeBuf(&bb);
		fclose(fp);
	}

/*	dispBufContents(&bb); */
	printf("\n");

	bufReader br;
	initReader(&br, &bb);
	int i;

	node *nd;
	makeTree(&nd, &br);
	printf("Tree:\n");
	dispNode(nd);
	printf("\n");

	freeBuf(&bb);

	initBuf(&bb, 4);

	nbits = 0;
	read_bytes = fread(&nbits, sizeof(int), 1, fp);
	printf("read %d bytes, nbits = %d\n", read_bytes, nbits);

	readFileToBuf(&bb, fp, nbits);
/*	dispBufContents(&bb); */
	printf("\n");

	bufReader txtbr;
	initReader(&txtbr, &bb);

	FILE *ofp = fopen(argv[2], "w");
	printf("Decoded text:\n");
	huffmanDecode(nd, &txtbr, nbits, ofp);

	freeBuf(&bb);

	freeNode(nd);
	free(nd);

	fclose(fp);
	fclose(ofp);

	return 0;
}

int checkFormat(FILE *fp) {
	unsigned char intendedFormat[10] = "\x00\x1aHUFF\xff\x0a";
	int i, corrupted = 0, invalid = 0;
	for (i = 0; i < 8; i++) {
		int ch = fgetc(fp);
		if (ch == EOF) {
			corrupted = 1;
			break;
		}
		printf("%02x ", intendedFormat[i]);
		unsigned char finalCh = (unsigned char) ch;
		if (finalCh != intendedFormat[i]) {
			invalid = 1;
			break;
		}
	}
	printf("\n");
	return corrupted || invalid;
}

void huffmanDecode(node *nd, bufReader *br, int nbits, FILE *ofp) {
	int nbytes = nbits / 8 + (nbits % 8 ? 1 : 0);
	node *temp = nd;
	while (br->curInd * 8 + br->curBind < nbits) {
		unsigned char bit = nextBit(br);
		if (bit) temp = temp->right;
		else temp = temp->left;
		if (temp->ch) {
			fputc(temp->ch, ofp);
			printf("%c", temp->ch);
			temp = nd;
		}
	}
	printf("\n");
}

void makeTree(node **nd, bufReader *br) {
	unsigned char bit = nextBit(br);
	node *newNd = (node *) malloc(sizeof(node));
	if (bit) {
		unsigned char byte = nextByte(br);
		newNd->left = newNd->right = NULL;
		newNd->ch = byte;
	}
	else {
		newNd->ch = '\0';
		makeTree(&newNd->left, br);
		makeTree(&newNd->right, br);
	}
	*nd = newNd;
}

void dispNode(node *nd) {
	if (nd->ch) {
		switch (nd->ch) {
			case ' ':
				printf("(' ') ");
				break;
			case '\n':
				printf("('\\n') ");
				break;
			default:
				printf("(%c) ", nd->ch);
		}
	}
	else {
		printf("Left: ( ");
		dispNode(nd->left);
		printf(") Right: ( ");
		dispNode(nd->right);
		printf(") ");
	}
}

void freeNode(node *nd) {
	if (!nd) return;
	if (nd->ch) return;
	if (nd->left->ch) {
		free(nd->left);
	}
	else {
		freeNode(nd->left);
		free(nd->left);
	}
	if (nd->right->ch) {
		free(nd->right);
	}
	else {
		freeNode(nd->right);
		free(nd->right);
	}
}

void initReader(bufReader *br, byteBuf *bb) {
	br->curInd = br->curBind = 0;
	br->bbuf = bb;
}

void advanceReader(bufReader *br) {
	br->curBind++;
	if (br->curBind > 7) {
		br->curInd++;
		br->curBind = 0;
	}
}

unsigned char nextBit(bufReader *br) {
	unsigned char byte = br->bbuf->bbuf[br->curInd];
/*	printf("Look at this byte I found: %08b\n", byte); */
	unsigned char bit = byte >> (7 - br->curBind);
	bit &= 0x01;
	advanceReader(br);
	return bit;
}

unsigned char nextByte(bufReader *br) {
	unsigned char byte = br->bbuf->bbuf[br->curInd];
	byte <<= br->curBind;
	int i;
	for (i = 0; i < 8; i++) advanceReader(br);
	if (br->curBind > 0 && br->curInd <= br->bbuf->idx) {
		unsigned char nextByte = br->bbuf->bbuf[br->curInd];
		nextByte >>= (8 - br->curBind);
		byte |= nextByte;
	}
	return byte;
}

void dispReader(bufReader *br) {
	printf("Indices: (%d, %d)\n", br->curInd, br->curBind);
}

int readFileToBuf(byteBuf *b, FILE *fp, int nbits) {
	int nbytes = nbits / 8 + (nbits % 8 ? 1 : 0);
	printf("nbytes = %d\n", nbytes);

	unsigned char finalCh = 0;
	int corrupted =  0;
	while (nbytes--) {
		int ch = fgetc(fp);
		if (ch == EOF) {
			corrupted = 1;
			break;
		}
		if (b->idx >= b->cap) {
			b->cap *= 2;
			b->bbuf = reallocarray(b->bbuf, b->cap, sizeof(char));
		}
		finalCh = (unsigned char) ch;
		b->bbuf[b->idx] = finalCh;
		b->idx++;
	}
	if (nbits % 8) b->idx--;
	b->bidx = nbits % 8;
	return corrupted;
}

void initBuf(byteBuf *b, int n) {
	b->cap = n;
	b->bbuf = (unsigned char *) calloc(n, sizeof(char));
	b->idx = b->bidx = 0;
}

void dispBufContents(byteBuf *b) {
	int i;
	for (i = 0; i < b->idx; i++) {
		printf("%08b ", b->bbuf[i]);
	}
	unsigned char mask = 0x80;
	int shift = 7;
	for (i = 0; i < b->bidx; i++) {
		unsigned char bit = (b->bbuf[b->idx] & mask) >> shift;
		printf("%d", bit);
		mask >>= 1; shift--;
	}
}

void freeBuf(byteBuf *b) {
	free(b->bbuf);
	b->idx = b->bidx = 0;
}

