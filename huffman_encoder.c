#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct n_t {
	unsigned char ch;
	char color;
	unsigned char is_leaf;
	int freq;
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
void huffmanEncode(char *, int, FILE *);

void freeNode(node *);

typedef struct {
	char bidx;	/* Bit index, value: [0, 7] */
	unsigned char bbuf;
} bitBuf;

int insertBBuf(bitBuf *, unsigned char);
void dispBBuf(bitBuf *);
void dispBBufContents(bitBuf *);
int decBBufInd(bitBuf *);
void copyBBuf(bitBuf *, bitBuf *);
void writeBBuf(bitBuf *, FILE *);

typedef struct {
	int cap;
	unsigned char *buf;
	int idx;
	bitBuf *bitb;	/* Honestly I could make it a struct instead of a ptr to a struct */
} byteBuf;

void initBuf(byteBuf *, int);
void insertBuf(byteBuf *, unsigned char);
void writeBuf(byteBuf *, FILE *);
void dispBuf(byteBuf *);
void dispBufContents(byteBuf *);
void freeBuf(byteBuf *);
void decBufInd(byteBuf *);
void copyBuf(byteBuf *, byteBuf *);
void appendBuf(byteBuf *, byteBuf *);

typedef struct {
	unsigned char ch;
	char color;
	byteBuf *buf;
} mapEntry;

void initMap(mapEntry **, int);
void freeMap(mapEntry **, int);
void dispMap(mapEntry **, int);
 
void genHuffCodes(node *, byteBuf *, int , char , mapEntry **, int); 
void serializeTree(node *nd, byteBuf *buf); 

void printEncodedText(char *, int, mapEntry **, int);

int main(int argc, char **argv) {
	printf("%d\n", BUFSIZ);
	char str[BUFSIZ];
	printf("sizeof = %lu\n", sizeof(str));

	if (argc != 2) {
		printf("Usage: %s filename\n", argv[0]);
		return 1;
	}

	int read_bytes = 0;
	int unpipedIn = isatty(0);
	if (unpipedIn) {
		printf("Sentence: ");
		fgets(str, BUFSIZ, stdin);
	}
	else {
		read_bytes = fread(str, 1, BUFSIZ, stdin);
		printf("Read bytes = %d\n", read_bytes);
		str[read_bytes] = '\0';
	}
	if (read_bytes == 0) {
		printf("Retard give me a full string\n");
		return 1;
	}
/*	if (unpipedIn && (str[strl - 1] == '\n')) {
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

	huffmanEncode(str, read_bytes, fp);
	fclose(fp);

	return 0;
}

int getFreqArr(char *str, int n, node ***chrnds) {
	int *freqs = (int *) calloc(256, sizeof(int));
	int i, nnzf = 0;

	for (i = 0; i < n; i++) {
		unsigned char ch = str[i];
		if (freqs[ch] == 0) {
			printf("%c : %02x\n", ch, ch);
			nnzf++;
		}
		freqs[ch]++;
	}
	printf("\n");

	printf("Number of non zero frequencies: %d\n", nnzf);

	node **chrnodes = (node **) malloc(nnzf * sizeof(node *));
	unsigned char curch;
	int x = 0;

	for (i = 0; i < 256; i++) {
		if (freqs[i] == 0) continue;
		curch = (unsigned char) i;
		node *chrn = (node *) malloc(sizeof(node));
		chrn->ch = curch;
		chrn->freq = freqs[i];
		chrn->left = chrn->right = NULL;
		chrn->is_leaf = 1;
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
	unsigned char ch = nd->ch;
	if (nd->is_leaf) {
		if (ch == ' ')  printf("' ': %d", nd->freq);
		else if (ch == '\n')  printf("'\\n': %d", nd->freq);
		else printf("(%x, %c): %d", ch, ch, nd->freq);
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
		pnd->is_leaf = 0;
		pnd->left = fm;
		pnd->right = sm;
		push(nds, hsize++, pnd);
	}
}

void genHuffCodes(node *nd, byteBuf *buf, int lvl, char dir, mapEntry **mp, int n) {
	unsigned char ch = nd->ch;
	int freq = nd->freq;
	if (lvl > 0) {
		insertBuf(buf, (unsigned char) dir - 48);
	}
	if (nd->is_leaf) {
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
	if (nd->is_leaf) {
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

void huffmanEncode(char *str, int slen, FILE *fp) {
	int i, n = slen, nnod;
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

	dispMap(mp, nnod);
	printEncodedText(str, n, mp, nnod);

	fputc(0x00, fp);
	fputc(0x1a, fp);
	fprintf(fp, "HUFF\xff\x0a");

	byteBuf treeBuf;
	initBuf(&treeBuf, 5);
	serializeTree(hufnd, &treeBuf);
	printf("The tree serialized: ");
	dispBuf(&treeBuf);
	printf("\n");
	writeBuf(&treeBuf, fp);

	freeBuf(&treeBuf);

	byteBuf hufbb;
	initBuf(&hufbb, 6);

	for (i = 0; i < n; i++) {
		unsigned char cur = str[i];
		int j, fInd = -1;
		for (j = 0; j < nnod; j++) {
			if (mp[j]->ch == cur) {
				fInd = j;
				break;
			}
		}
		if (fInd < 0) continue;
		appendBuf(&hufbb, mp[fInd]->buf);
	}

	writeBuf(&hufbb, fp);

	freeBuf(&hufbb);
	
	freeMap(mp, nnod);
	free(mp);


	freeNode(hufnd);
	free(hufnd);
	free(chrnodes);
}

void freeNode(node *nd) {
	if (!nd) return;
	if (nd->is_leaf) return;
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

int insertBBuf(bitBuf *bitb, unsigned char bit) {
	int resetSignal = 0;
	unsigned char byte = 1 << (7 - bitb->bidx);
	/* reset */
	bitb->bbuf &= ~byte;
	/* insert */
	bitb->bbuf |= (bit << (7 - bitb->bidx));
	bitb->bidx++;

	if (bitb->bidx >= 8) {
		bitb->bidx = 0;
		resetSignal = 1;
	}
	return resetSignal;
}

int decBBufInd(bitBuf *bitb) {
	bitb->bidx--;
	if (bitb->bidx < 0) {
		bitb->bidx = 7;
		printf("Going back to prev byte\n");
		return 1;
	}
	return 0;
}

void dispBBuf(bitBuf *bitb) {
	printf("Bit index: %d\n", bitb->bidx);
	printf("Contents: ");
	dispBBufContents(bitb);
	printf("\n");
}
void dispBBufContents(bitBuf *bitb) {
	if (bitb->bidx <= 0) return;
	unsigned char mask = 0x80;
	int shift = 7, i;
	for (i = 0; i < bitb->bidx; i++) {
		unsigned char bit = (bitb->bbuf & mask) >> shift;
		printf("%d", bit);
		mask >>= 1; shift--;
	}
}

void copyBBuf(bitBuf *srcbb, bitBuf *dstbb) {
	dstbb->bidx = srcbb->bidx;
	dstbb->bbuf = srcbb->bbuf;
}

void writeBBuf(bitBuf *bitb, FILE *fp) {
	if (bitb->bidx <= 0) return;
	fputc(bitb->bbuf, fp);
}

void initBuf(byteBuf *b, int n) {
	b->cap = n;
	b->buf = (unsigned char *) calloc(n, sizeof(char));
	b->idx = 0;
	bitBuf *bitb = (bitBuf *) malloc(sizeof(bitBuf));
	bitb->bidx = 0;
	bitb->bbuf = 0;
	b->bitb = bitb;
}

void dispBuf(byteBuf *b) {
	printf("Current index: (%d, %d) or (%x, %x)\n", b->idx, b->bitb->bidx, b->idx, b->bitb->bidx);
	dispBufContents(b);
	printf("\n");
}

void dispBufContents(byteBuf *b) {
	int i;
	for (i = 0; i < b->idx; i++) {
		printf("%08b", b->buf[i]);
	}
	dispBBufContents(b->bitb);
}

void insertBuf(byteBuf *b, unsigned char bit) {
	int reset = insertBBuf(b->bitb, bit);
	if (reset) {
		if (b->idx >= b->cap) {
			b->cap *= 2;
			b->buf = (unsigned char *) reallocarray(b->buf, b->cap, sizeof(char));
		}	
		b->buf[b->idx] = b->bitb->bbuf;
		b->bitb->bbuf = 0;
		b->idx++;
	}
}


void writeBuf(byteBuf *b, FILE *fp) {
	fwrite(&b->idx, sizeof(int), 1, fp);
	fwrite(&b->bitb->bidx, sizeof(unsigned char), 1, fp);
	int i;
	for (i = 0; i < b->idx; i++) {
		fputc(b->buf[i], fp);
	}
	writeBBuf(b->bitb, fp);
}

void freeBuf(byteBuf *b) {
	free(b->buf);
	free(b->bitb);
	b->idx = 0;
}
void decBufInd(byteBuf *b) {
	int indDec = decBBufInd(b->bitb);
	if (indDec) {
		b->idx--;
		b->bitb->bbuf = b->buf[b->idx];
	}
	if (b->idx < 0) exit(1);
}
void copyBuf(byteBuf *srcb, byteBuf *dstb) {
	/* allocate same space as srcb */
	int cap = srcb->cap, i;
	dstb->cap = cap;
	dstb->buf = (unsigned char *) reallocarray(dstb->buf, cap, sizeof(unsigned char));
	for (i = 0; i < srcb->idx; i++) {
		dstb->buf[i] = srcb->buf[i];
	}
	dstb->idx = i;
	copyBBuf(srcb->bitb, dstb->bitb);
}

void appendBuf(byteBuf *hostBb, byteBuf *elemBb) {
	int i;
	for (i = 0; i < elemBb->idx; i++) {
		int j, shift = 7;
		unsigned char mask = 0x80, byte = elemBb->buf[i];
		for (; shift >= 0; shift--) {
			unsigned char bit = (byte & mask) >> shift;
			mask >>= 1;
			insertBuf(hostBb, bit);
		}
	}
	unsigned char mask = 0x80, byte = elemBb->bitb->bbuf;
	int shift = 7;
	for (i = 0; i < elemBb->bitb->bidx; i++) {
		unsigned char bit = (byte & mask) >> shift;
		insertBuf(hostBb, bit);
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

void dispMap(mapEntry **mp, int n) {
	int i;
	printf("Lookup table:\n");
	for (i = 0; i < n; i++) {
		unsigned char mpch = mp[i]->ch;
		switch (mpch) {
			case '\n':
				printf("'\\n': ");
				break;
			case ' ':
				printf("' ': ");
				break;
			default:
				printf("(%x, %c): ", mpch, mpch);
		}
		printf("\x1b[%dm", mp[i]->color);
		dispBufContents(mp[i]->buf);
		printf("\x1b[0m");
		printf("\n");
	}

}

void printEncodedText(char *str, int strl, mapEntry **mp, int n) {
	printf("Encoded text: ");
	int i, j;
	for (i = 0; i < strl; i++) {
		unsigned char ch = str[i];
		int fInd = -1;
		for (j = 0; j < n; j++) {
			if (mp[j]->ch == ch) {
				fInd = j;
				break;
			}
		}
		if (fInd == -1) break;
		printf("\x1b[%dm", mp[fInd]->color);
		dispBufContents(mp[fInd]->buf);
	}
	printf("\x1b[0m\n");
}

void freeMap(mapEntry **mp, int n) {
	int i;
	for (i = 0; i < n; i++) {
		freeBuf(mp[i]->buf);
		free(mp[i]->buf);
		free(mp[i]);
	}
}
