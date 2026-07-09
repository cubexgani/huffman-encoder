#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct n_t {
	char ch;
	int freq;
	char color;
	struct n_t *left, *right;
} node;

typedef struct {
	node *head;
} hufTree;

void heapify(node **, int, int);
void buildHeap(node **, int);
void push(node **, int, node *);
void increaseKey(node **, int, int, node *);
node * top(node **, int);
node * pop(node **, int);

void display(node **, int);
void dispNode(node *);
int getFreqArr(char *, int, node ***);

void genHuffTree(node **, int);
void huffmanEncode(char *, FILE *);

void freeNode(node *);

typedef struct {
	int cap;
	unsigned char *bbuf;
	int bidx, idx;
} byteBuf;

void initBuf(byteBuf *, int);
void insertBuf(byteBuf *, unsigned char);
void writeBuf(byteBuf *, FILE *);
void dispBuf(byteBuf *);
void freeBuf(byteBuf *);
void decBufInd(byteBuf *);
void copyBuf(byteBuf *, byteBuf *);
void dispBufContents(byteBuf *);
void serializeTree(node *nd, byteBuf *buf); 

typedef struct {
	char ch;
	char color;
	byteBuf *buf;
} mapEntry;

void initMap(mapEntry **, int);
void freeMap(mapEntry **, int);
 
void genHuffCodes(node *, byteBuf *, int , char , mapEntry **, int); 

int main(int argc, char **argv) {
	printf("%d\n", BUFSIZ);
	char str[BUFSIZ];
	printf("sizeof = %lu\n", sizeof(str));

	if (argc != 2) {
		printf("Usage: %s filename\n", argv[0]);
		return 1;
	}

	int read_bytes = 0;
	if (isatty(0)) {
		printf("Sentence: ");
		fgets(str, BUFSIZ, stdin);
	}
	else {
		read_bytes = fread(str, 1, BUFSIZ, stdin);
		printf("Read bytes = %d\n", read_bytes);
		str[read_bytes] = '\0';
	}
	int strl = strlen(str);
	if (strl == 0) {
		printf("Retard give me a full string\n");
		return 1;
	}
/*	if (str[strl - 1] == '\n') {
		str[strl - 1] = '\0';
		strl--;
	} */
	printf("%s\n", str);

	FILE *fp = fopen(argv[1], "w");
	if (!fp) {
		printf("Some error occurred while opening file\n");
		return 1;
	}

	int i;

	huffmanEncode(str, fp);
	fclose(fp);
	return 0;
}

int getFreqArr(char *str, int n, node ***chrnds) {
	
	/* Supports all 128 ASCII characters */
	int *freqs = (int *) calloc(256, sizeof(int));
	int i, nnzf = 0;

	for (i = 0; i < n; i++) {
		char ch = str[i];
		if (freqs[ch] == 0) nnzf++;
		freqs[ch]++;
	}

	printf("Number of non zero frequencies: %d\n", nnzf);

	node **chrnodes = (node **) malloc(nnzf * sizeof(node *));
	char curch;
	int x = 0;

	for (i = 0; i < 256; i++) {
		if (freqs[i] == 0) continue;
		curch = (char) i;
		node *chrn = (node *) malloc(sizeof(node));
		chrn->ch = curch;
		chrn->freq = freqs[i];
		chrn->left = chrn->right = NULL;
		chrnodes[x++] = chrn;
	}

	free(freqs);

	char colind = 1, colbase = 30;
	for (i = 0; i < nnzf; i++) {
		chrnodes[i]->color = colind + colbase;
		colind++;
		if (colind == 7) {
			colbase = colbase == 30 ? 90 : 30;
			colind = 1;
		}
	}

	*chrnds = chrnodes;
	return nnzf;
}

void dispNode(node *nd) {
	char ch = nd->ch;
	if (ch) {
		if (ch == ' ')  printf("' ': %d", nd->freq);
		else if (ch == '\n')  printf("'\\n': %d", nd->freq);
		else printf("%c: %d", ch, nd->freq);
	}
	else {
		printf("Parent freq: %d, ", nd->freq);
		printf("Left: (");
		dispNode(nd->left);
		printf(") Right: (");
		dispNode(nd->right);
		printf(") ");
	}
}		

void display(node **nds, int n) {
	int i;
	for (i = 0; i < n; i++) {
		node *chnd = nds[i];
		dispNode(chnd);
		printf("\n");
	}
}

void buildHeap(node **nds, int n) {
	int i;
	for (i = n / 2 - 1; i >= 0; i--) heapify(nds, n, i);
}
void heapify(node **nds, int n, int i) {
	if (n == 0) return;
	int left = 2 * i + 1, right = 2 * i + 2;
	int smallest = i;
	if (left < n && nds[left]->freq < nds[i]->freq) smallest = left;
	else smallest = i;
	if (right < n && nds[right]->freq < nds[smallest]->freq) smallest = right;
	if (smallest != i) {
		node *temp = nds[i];
		nds[i] = nds[smallest];
		nds[smallest] = temp;
		heapify(nds, n, smallest);
	}
}
void push(node **nds, int n, node *nd) {
	nds[n] = nd;
	increaseKey(nds, n, n, nd);
}
void increaseKey(node **nds, int n, int i, node *nd) {
	if (nd->freq < nds[i]->freq) return;
	int p = (i - 1) / 2;
	while (i > 0 && nds[p]->freq > nds[i]->freq) {
		node *temp = nds[i];
		nds[i] = nds[p];
		nds[p] = temp;
		i = p;
		p = (i - 1) / 2;
	}
}
node * top(node **nds, int n) {
	if (n == 0) return NULL;
	return nds[0];
}
node * pop(node **nds, int n) {
	if (n == 0) return NULL;
	node *max = nds[0];
	nds[0] = nds[n - 1];
	heapify(nds, n - 1, 0);
	return max;
}

void genHuffTree(node **nds, int n) {
	int hsize = n, x = 1;
	while (hsize > 1) {
		node *fm = pop(nds, hsize--);
		node *sm = pop(nds, hsize--);
		node *pnd = (node *) malloc(sizeof(node));
		pnd->ch = 0;
		pnd->freq = fm->freq + sm->freq;
		pnd->left = fm;
		pnd->right = sm;
		push(nds, hsize++, pnd);
	}
}

void genHuffCodes(node *nd, byteBuf *buf, int lvl, char dir, mapEntry **mp, int n) {
	char ch = nd->ch;
	int freq = nd->freq;
	if (lvl > 0) {
		insertBuf(buf, (unsigned char) dir - 48);
	}
	if (ch) {
		int find = -1, i;
		for (i = 0; i < n; i++) {
			if (mp[i]->ch == ch) {
				find = i;
				break;
			}
		}
		if (find == -1) return;
		copyBuf(buf, mp[i]->buf);
		return;
	}
	genHuffCodes(nd->left, buf, lvl + 1, '0', mp, n);
	decBufInd(buf);
	genHuffCodes(nd->right, buf, lvl + 1, '1', mp, n);
	decBufInd(buf);
}

void serializeTree(node *nd, byteBuf *buf) {
	if (nd->ch) {
		insertBuf(buf, 1);
		unsigned char mask = 0x80, byte = nd->ch;
		int shift = 7;
		for (; shift >= 0; shift--) {
			unsigned char bit = (byte & mask) >> shift;
			insertBuf(buf, bit);
			mask >>= 1;
		}
	}
	else {
		insertBuf(buf, 0);
		serializeTree(nd->left, buf);
		serializeTree(nd->right, buf);
	}
}

void huffmanEncode(char *str, FILE *fp) {
	int i, n = strlen(str), nnod;
	printf("strlen = %d\n", n);
	node **chrnodes;
	nnod = getFreqArr(str, n, &chrnodes);

	mapEntry **mp = (mapEntry **) malloc(nnod * sizeof(mapEntry *));
	initMap(mp, nnod);
	for (i = 0; i < nnod; i++) {
		mp[i]->ch = chrnodes[i]->ch;
		mp[i]->color = chrnodes[i]->color;
	}

	buildHeap(chrnodes, nnod);
	genHuffTree(chrnodes, nnod);
	node *hufnd = top(chrnodes, nnod);

	printf("Huffman tree:\n");
	dispNode(hufnd);
	printf("\n");

	byteBuf bb;
	initBuf(&bb, 2);
	
	genHuffCodes(hufnd, &bb, 0, 0, mp, nnod);
	freeBuf(&bb);

	/*
	printf("Lookup table:\n");
	for (i = 0; i < nnod; i++) {
		char mpch = mp[i]->ch;
		switch (mpch) {
			case '\n':
				printf("'\\n': ");
				break;
			case ' ':
				printf("' ': ");
				break;
			default:
				printf("%c: ", mpch);
		}
		printf("\x1b[%dm", mp[i]->color);
		dispBufContents(mp[i]->buf);
		printf("\x1b[0m");
		printf("\n");
	}
	*/

	fputc(0x00, fp);
	fputc(0x1a, fp);
	fprintf(fp, "HUFF\xff\x0a");

	byteBuf treeBuf;
	initBuf(&treeBuf, 5);
	serializeTree(hufnd, &treeBuf);
/*	printf("The tree serialized: ");
	dispBufContents(&treeBuf);
	printf("\n"); */
	writeBuf(&treeBuf, fp);

	freeBuf(&treeBuf);

	byteBuf hufbb;
	initBuf(&hufbb, 6);

	for (i = 0; i < n; i++) {
		char cur = str[i];
		int j, fInd = -1;
		for (j = 0; j < nnod; j++) {
			if (mp[j]->ch == cur) {
				fInd = j;
				break;
			}
		}
		if (fInd < 0) continue;
		copyBuf(mp[fInd]->buf, &hufbb);
	}

/*	printf("The text serialized: ");
	dispBufContents(&hufbb);
	printf("\n"); */
	writeBuf(&hufbb, fp);

	freeBuf(&hufbb);
	
	freeMap(mp, nnod);
	free(mp);


/*	FILE *tfp = fopen("hufftree.bin", "w");
	if (tfp) writeBuf(&treeBuf, tfp); */

	freeNode(hufnd);
	free(hufnd);
	free(chrnodes);
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

void initBuf(byteBuf *b, int n) {
	b->cap = n;
	b->bbuf = (unsigned char *) calloc(n, sizeof(char));
	b->idx = b->bidx = 0;
}

void dispBuf(byteBuf *b) {
	printf("Data: %08b\n", b->bbuf[b->idx]);
	printf("Current index: (%d, %d)\n", b->idx, b->bidx);
	dispBufContents(b);
	printf("\n");
}

void dispBufContents(byteBuf *b) {
	int i;
	for (i = 0; i < b->idx; i++) {
		printf("%08b", b->bbuf[i]);
	}
	unsigned char mask = 0x80;
	int shift = 7;
	for (i = 0; i < b->bidx; i++) {
		unsigned char bit = (b->bbuf[b->idx] & mask) >> shift;
		printf("%d", bit);
		mask >>= 1; shift--;
	}
}

void insertBuf(byteBuf *b, unsigned char bit) {
	if (b->idx >= b->cap) {
		b->cap *= 2;
		b->bbuf = (unsigned char *) reallocarray(b->bbuf, b->cap, sizeof(char));
		b->bbuf[b->idx] = 0;
	}
	unsigned char byte = 1 << (7 - b->bidx);
	/* reset */
	b->bbuf[b->idx] &= ~byte;
	/* insert */
	b->bbuf[b->idx] |= (bit << (7 - b->bidx));
	b->bidx++;
	if (b->bidx > 7) {
		b->bidx = 0;
		b->idx++;
		if (b->idx < b->cap) b->bbuf[b->idx] = 0;
	}
}

void writeBuf(byteBuf *b, FILE *fp) {
	int nbits = b->idx * 8 + b->bidx;
/*	printf("No. of bits = %d aka %x or %032b\n", nbits, nbits, nbits); */
	fwrite(&nbits, sizeof(int), 1, fp);
	int i;
	for (i = 0; i < b->idx; i++) {
		fputc(b->bbuf[i], fp);
	}
	if (b->bidx) fputc(b->bbuf[b->idx], fp);
}

void freeBuf(byteBuf *b) {
	free(b->bbuf);
	b->idx = b->bidx = 0;
}

void decBufInd(byteBuf *b) {
	b->bidx--;
	if (b->bidx < 0) {
		b->idx--;
		b->bidx = 7;
	}
	if (b->idx < 0) exit(1);
}

void copyBuf(byteBuf *srcb, byteBuf *dstb) {
	int i;
	for (i = 0; i < srcb->idx; i++) {
		unsigned char byte = srcb->bbuf[i];
		unsigned char mask = 0x80;
		int shift = 7, j;
		for (j = 0; j < 8; j++) {
			unsigned char bit = (byte & mask) >> shift;
			insertBuf(dstb, bit);
			mask >>= 1; shift--;
		}
	}
	unsigned char mask = 0x80;
	int shift = 7;
	for (i = 0; i < srcb->bidx; i++) {
		unsigned char bit = (srcb->bbuf[srcb->idx] & mask) >> shift;
		insertBuf(dstb, bit);
		mask >>= 1; shift--;
	}
}

void initMap(mapEntry **mp, int n) {
	int i;
	for (i = 0; i < n; i++) {
		mp[i] = (mapEntry *) malloc(sizeof(mapEntry));
		mp[i]->ch = '\0';
		mp[i]->buf = (byteBuf *) malloc(sizeof(byteBuf));
		initBuf(mp[i]->buf, 2);
	}
}

void freeMap(mapEntry **mp, int n) {
	int i;
	for (i = 0; i < n; i++) {
		freeBuf(mp[i]->buf);
		free(mp[i]->buf);
		free(mp[i]);
	}
}
