#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#define norw 12 //�����ָ���
#define id_max 15//��ʶ�����ĳ��� 
#define number_max 14//�������ĳ��� 
#define table_max 100//���ű����� 
#define cxmax 500//��������������
#define amax 2048     /* ��ַ�Ͻ�*/
#define stacksize 5000 //����ջ��󳤶�

enum symbol {
	nul, ident, number, plus, minus, times, slash, lparen, rparen, ls, rs, eql, neq, rss, rsq, lsq, lss, semicolon, becoms, beginsym, endsym,
	repeatsym, readsym, writesym, assignmentsym, thensym, ifsym, untilsym, elsesym, arrsym
};
struct tablestruct {
	char name[id_max];
	int addr;//�����ĵ�ַ��������ĵ�ַ
	int kind = 0;//0��ʾ������1��ʾһά���飬2��ʾ2ά����
	int dix1 = 0;
	int dix2 = 0;
}; //���ű�ṹ


/* ���������ָ�� */
enum fct {
	lit, opr, lod,
	sto, cmp, ini,
	jmp, jpc, getarr, create_arr, store_arr, load_arr//ȡ���� 
};


#define fctnum 13
/* ���������ṹ */
struct instruction
{
	enum fct f; /* ���������ָ�� */
	int l;      /* ���ò���������Ĳ�β� */
	int a;      /* ����f�Ĳ�ͬ����ͬ */
};



struct tablestruct table[table_max];//���ű� 
char ch;//����getch
enum symbol sym;//��ǰ����
int cx;             /* ���������ָ��, ȡֵ��Χ[0, cxmax-1]*/
int cc, ll;         //getchʹ�õļ�������cc��ʾ��ǰ�ַ�(ch)��λ�� 
char line[81];      /* ��ȡ�л����� */
char word[norw][id_max];        /* ������ */
enum symbol wsym[norw];     /* �����ֶ�Ӧ�ķ���ֵ */
enum symbol ssym[256];      /* ���ַ��ķ���ֵ */
char a[id_max + 1];//��ʱ����
char id[id_max + 1];
int num;//��ǰ���� 
struct instruction code[cxmax]; /* ����������������� */
char mnemonic[fctnum][20];   /* ���������ָ������ */

FILE* fin;
FILE* fcode;
FILE* fresult;

int error = -1;//������,1��ʾ����λ��̫�࣬2��ʾ���ű���,3��ʾδʶ��ı�����
		 //4��ʾ����û������,5��ʾ�޷�ʶ��


void init();
void getch();
void getsym();
//ptxָ���β 
void program(int* ptx);
void stmt_sequence(int* ptx);
void statement(int* ptx);
void exp(int* ptx);
void simple_exp(int* ptx);
void term(int* ptx);
void factor(int* ptx);
void enter(int* ptx, int addr);
void listcode(int cx0)
{
	int i;
	printf("\n");
	for (i = cx0; i < cx; i++)
	{
		printf("%d %s %d %d\n", i, mnemonic[code[i].f], code[i].l, code[i].a);
	}
}
void listall()
{
	int i;
	for (i = 0; i < cx; i++)
	{
		printf("%d %s %d %d\n", i, mnemonic[code[i].f], code[i].l, code[i].a);
		fprintf(fcode, "%d %s %d %d\n", i, mnemonic[code[i].f], code[i].l, code[i].a);
	}
}
int position(char* id_, int tx)//���ұ�ʶ���ڱ��е�λ��,-1ʱ����
{
	if (tx < 0)
		return -1;
	int i;
	i = tx;
	while (strcmp(table[i].name, id) != 0 && i >= 0)
	{
		i--;
	}
	return i;
}
void gen(enum fct x, int y, int z)
{
	if (cx >= cxmax)
	{
		printf("Program is too long!\n");	/* ���ɵ���������������� */
		exit(1);
	}
	if (z >= amax)
	{
		printf("Displacement address is too big!\n");	/* ��ַƫ��Խ�� */
		exit(1);
	}
	code[cx].f = x;
	code[cx].l = y;
	code[cx].a = z;
	cx++;
}
void interpret();

//��ʼ��
void init()
{
	//���õ��ַ� 
	for (int i = 0; i < 256; i++)
		ssym[i] = nul;
	ssym['+'] = plus;
	ssym['/'] = slash;
	ssym['*'] = times;
	ssym['-'] = minus;
	ssym['('] = lparen;
	ssym[')'] = rparen;
	ssym[';'] = semicolon;
	ssym['['] = ls;
	ssym[']'] = rs;
	//���ñ���������
	strcpy(&(word[0][0]), "begin");
	strcpy(&(word[1][0]), "end");
	strcpy(&(word[2][0]), "repeat");
	strcpy(&(word[3][0]), "read");
	strcpy(&(word[4][0]), "write");
	strcpy(&(word[5][0]), "assignment");
	strcpy(&(word[6][0]), "then");
	strcpy(&(word[7][0]), "if");
	strcpy(&(word[8][0]), "until");
	strcpy(&(word[9][0]), "else");
	strcpy(&(word[10][0]), "arr");
	//���ñ����ַ���
	wsym[0] = beginsym;
	wsym[1] = endsym;
	wsym[2] = repeatsym;
	wsym[3] = readsym;
	wsym[4] = writesym;
	wsym[5] = assignmentsym;
	wsym[6] = thensym;
	wsym[7] = ifsym;
	wsym[8] = untilsym;
	wsym[9] = elsesym;
	wsym[10] = arrsym;
	//ָ�
	strcpy(&(mnemonic[lit][0]), "lit");
	strcpy(&(mnemonic[opr][0]), "opr");
	strcpy(&(mnemonic[lod][0]), "lod");
	strcpy(&(mnemonic[sto][0]), "sto");
	strcpy(&(mnemonic[cmp][0]), "cmp");
	strcpy(&(mnemonic[ini][0]), "int");
	strcpy(&(mnemonic[jmp][0]), "jmp");
	strcpy(&(mnemonic[jpc][0]), "jpc");
	strcpy(&(mnemonic[getarr][0]), "getarr");
	strcpy(&(mnemonic[create_arr][0]), "create_arr");
	strcpy(&(mnemonic[store_arr][0]), "store_arr");
	strcpy(&(mnemonic[load_arr][0]), "load_arr");
}
void getch()
{
	if (cc == ll) /* �жϻ��������Ƿ����ַ��������ַ����������һ���ַ����������� */
	{
		if (feof(fin))
		{
			printf("Program is incomplete!\n");
			//exit(1);
		}
		ll = 0;
		cc = 0;
		/*printf("%d ", cx);
		fprintf(foutput,"%d ", cx);*/
		ch = ' ';
		while (ch != 10)
		{
			if (EOF == fscanf(fin, "%c", &ch))
			{
				line[ll] = 0;
				break;
			}

			printf("%c", ch);
			//fprintf(foutput, "%c", ch);
			line[ll] = ch;
			ll++;
		}
	}
	ch = line[cc];
	cc++;
}
void getsym()//�ʷ����� 
{
	int i, j, k;
	while (ch == ' ' || ch == 10 || ch == 9)	/* ���˿ո񡢻��к��Ʊ�� */
	{
		getch();
	}
	if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) /* ��ǰ�ĵ����Ǳ�ʶ�����Ǳ����� */
	{
		k = 0;
		do {
			if (k < id_max)
			{
				a[k] = ch;
				k++;
			}
			getch();
		} while ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9'));
		a[k] = 0;
		strcpy(id, a);
		i = 0;
		j = norw - 1;
		int tag = 0;
		while (i <= j)
		{
			if (strcmp(id, word[i]) == 0)
			{
				tag = 1;
				break;
			}
			i++;
		}
		if (tag == 1) /* ��ǰ�ĵ����Ǳ����� */
		{
			sym = wsym[i];
			//printf("�Ǳ�����\n");
		}
		else /* ��ǰ�ĵ����Ǳ�ʶ�� */
		{
			sym = ident;
			//printf("�Ǳ�ʶ�� %d\n",(sym==ident));
		}
	}
	else
	{
		if (ch >= '0' && ch <= '9') /* ��ǰ�ĵ��������� */
		{
			k = 0;
			num = 0;
			sym = number;
			do {
				num = 10 * num + ch - '0';
				k++;
				getch();;
			} while (ch >= '0' && ch <= '9'); /* ��ȡ���ֵ�ֵ */
			k--;
			//printf("���ֵ�ֵ��%d\n",num);
			if (k > number_max) /* ����λ��̫�� */
			{
				error = 1;//����λ��̫��
			}
		}
		else
		{
			if (ch == ':')		/* ��⸳ֵ���� */
			{
				getch();
				if (ch == '=')
				{
					sym = becoms;
					//printf("�Ǹ�ֵ��\n");
					getch();
				}
				else
				{
					sym = nul;	/* ����ʶ��ķ��� */
				}
			}
			else
			{
				if (ch == '<')		/* ���С�ڻ�С�ڵ��ڷ��� */
				{
					getch();
					if (ch == '=')
					{
						sym = lsq;
						getch();
					}
					else
					{
						sym = lss;
					}
				}
				else
				{
					if (ch == '>')		/* �����ڻ���ڵ��ڷ��� */
					{
						getch();
						if (ch == '=')
						{
							sym = rsq;
							getch();
						}
						else
						{
							sym = rss;
						}
					}
					else if (ch == '=')
					{
						getch();
						if (ch == '=')
						{
							sym = eql;
							getch();
						}
						else
						{
							sym = nul;
						}
					}
					else if (ch == '!')
					{
						getch();
						if (ch == '=')
						{
							sym = neq;
							getch();
						}
						else
						{
							sym = nul;
						}
					}
					else
					{
						sym = ssym[ch];
						getch();
						//printf("�ǵ��ַ�\n");	/* �����Ų�������������ʱ��ȫ�����յ��ַ����Ŵ��� */                   
					}
				}
			}
		}
	}
}
void enter(int* ptx, int add)//����ǰ�ķ��ż�����ű� 
{
	(*ptx)++;
	if (*ptx >= table_max)
		error = 2;//���ű���
	else
	{
		strcpy(table[(*ptx)].name, id);
		table[(*ptx)].addr = add;
	} /* ���ű��name���¼��ʶ�������� */
}
void program(int* ptx)
{
	stmt_sequence(ptx);
}
void stmt_sequence(int* ptx)
{
	statement(ptx);
	while (sym == semicolon)
	{
		getsym();
		statement(ptx);
	}
}
void statement(int* ptx)
{
	printf("%d", sym);
	if (sym == ifsym)//if---else ���
	{
		getsym();
		exp(ptx);//������ʽ
		int now1 = cx;
		gen(jpc, 0, -1);//���������ת��-1��ʾ������
		if (sym == thensym)
		{
			getsym();
			stmt_sequence(ptx);
			int now2 = cx;
			gen(jmp, 0, -1);//��������תָ��,������
			code[now1].a = cx;
			if (sym == elsesym)
			{
				getsym();
				stmt_sequence(ptx);
				code[now2].a = cx;
			}
			else
			{
				code[now2].a = cx;
			}
			if (sym == endsym)
			{
				getsym();
			}
			else error = 3;
		}
		else error = 3;
	}
	else if (sym == repeatsym)//repeat���
	{
		getsym();
		int now1 = cx;
		gen(jmp, 0, -1);//��������תָ�������
		stmt_sequence(ptx);
		code[now1].a = cx;//����
		if (sym == untilsym)
		{
			getsym();
			exp(ptx);
			gen(jpc, 0, now1 + 1);//����������ת
		}
	}
	else if (sym == ident)//assign���
	{

		int i = position(id, *ptx);
		if (i == -1)//���ڷ��ű��� 
		{
			enter(ptx, -1);
			i = position(id, *ptx);
			table[i].kind = 0;
			gen(ini, 0, i);
		}//������ű�,��ַ��ʼ��-1
		i = position(id, *ptx);
		if (table[i].kind == 0)//�Ǳ���
		{
			getsym();
			if (sym == becoms)
			{
				getsym();
				exp(ptx);
				gen(sto, 0, i);
			}
			else error = 3;
		}
		else
		{
			int x, y;
			getsym();
			if (sym == ls)
			{
				getsym();
				if (sym == number)
				{
					x = num;
					getsym();
					if (sym == rs)
					{
						getsym();
						if (sym == ls)
						{
							getsym()
								;
							if (sym == number)
							{
								y = num;
								getsym();
								if (sym == rs)
								{
									getsym();
									if (sym == becoms)
									{
										//��ά���鸳ֵ����
										//����exp
										getsym();
										exp(ptx);
										//����ַ��ջ��getarr��
										gen(getarr, 0, i);
										//x��ջ
										gen(lit, 0, x);
										//size��ջ
										gen(lit, 0, table[i].dix2);
										//�����ַ
										gen(opr, 0, 11);
										gen(opr, 0, 9);
										//y��ջ
										gen(lit, 0, y);
										gen(opr, 0, 9);

										//��ţ�store_arr��
										gen(store_arr, 0, 0);//Ϊ0��ʾ����վ��ѹ��ջ����ָ��Ŀռ�
									}
								}
								else error = 7;
							}
							else
								error = 7;
						}
						else if (sym == becoms)
						{
							//һά���鸳ֵ����
							//����exp
							getsym();
							exp(ptx);
							//����ַ��ջ��getarr��
							gen(getarr, 0, i);
							//x��ջ
							gen(lit, 0, x);
							//size��ջ
							//�����ַ
							gen(opr, 0, 9);

							//��ţ�store_arr��
							gen(store_arr, 0, 0);//Ϊ0��ʾ����վ��ѹ��ջ����ָ��Ŀռ�
						}
						else
							error = 7;
					}
					else
						error = 7;
				}
				else
				{
					error = 7;
				}
			}
			else
			{
				error = 7;
			}
		}
		/*getsym();
		if (sym == becoms)
		{
			getsym();
			exp(ptx);
			gen(sto, 0, i);
		}
		else error = 3;*/
	}
	else if (sym == writesym)//write���
	{
		getsym();
		exp(ptx);
		gen(opr, 0, 1);//1��ʾ���

	}
	else if (sym == readsym)//read���
	{
		getsym();
		if (sym == ident)
		{
			int i = position(id, *ptx);//Ҫ���ı�����λ��
			if (i == -1)
				error = 4;//����û������
			if (i != -1)
			{
				if (table[i].kind == 0)
				{
					gen(lod, 0, i);//���������ص�����ջ
					gen(opr, 0, 2);
				}//2��ʾ���ջ��Ԫ��
				else if (table[i].kind == 1)
				{
					int x;
					getsym();
					if (sym == ls)
					{
						getsym();
						if (sym == number)
						{
							x = num;
							getsym();
							if (sym == rs)
							{
								//����һά����
								gen(getarr, 0, i);
								gen(lit, 0, x);
								gen(opr, 0, 9);
								gen(load_arr, 0, 0);
								gen(opr, 0, 2);
							}
							else error = 8;
						}
						else error = 8;
					}
					else error = 8;
				}
				else
				{
					int dix1, dix2;
					getsym();
					if (sym == ls)
					{
						getsym();
						if (sym == number)
						{
							dix1 = number;
							getsym();
							if (sym == rs)
							{
								getsym();
								if (sym == ls)
								{
									getsym();
									if (sym == number)
									{
										dix2 = num;
										getsym()
											;
										if (sym == rs)
										{
											//�����ά����
											gen(getarr, 0, i);
											gen(lit, 0, dix1);
											gen(lit, 0, table[i].dix2);
											gen(opr, 0, 11);
											gen(opr, 0, 9);
											gen(lit, 0, dix2);
											gen(opr, 0, 9);
											gen(load_arr, 0, 0);
											gen(opr, 0, 2);
										}
										else error = 8;
									}
									else error = 8;
								}
								else
								{
									error = 8;
								}
							}
							else error = 8;
						}
						else error = 8;
					}
					else error = 8;
				}

			}
			getsym();
		}
	}
	else if (sym == arrsym)//������������
	{
		getsym();
		enter(ptx, -1);//���������������ű�
		int i = position(id, *ptx);//�õ�λ��
		getsym();
		int dix1, dix2, kind;
		if (sym == ls)
		{
			getsym();
			if (sym == number)
			{
				dix1 = num;
				getsym();
				if (sym == rs)
				{
					getsym();
					if (sym == ls)
					{
						getsym();
						if (sym == number)
						{
							dix2 = num;
							getsym();
							if (sym == rs)
							{
								//��ά����
								table[i].dix1 = dix1;
								table[i].dix2 = dix2;
								table[i].kind = 2;
								gen(create_arr, 0, i);
								getsym();
							}
							else
							{
								error = 6;
							}

						}
						else
						{
							error = 6;
						}
					}
					else
					{
						//һά����
						table[i].dix1 = dix1;
						table[i].kind = 1;
						gen(create_arr, 0, i);//���ٿռ� 
					}
				}
				else
				{
					error = 6;
				}
			}
			else
			{
				error = 6;
			}
		}
		else
		{
			error = 6;//������������
		}
	}
	else { error = 3; }//δʶ��ı�����
}
void exp(int* ptx)
{
	simple_exp(ptx);
	if (sym == lss || sym == lsq || sym == rss || sym == rsq || sym == neq || sym == eql)
	{
		int tag = 0;
		if (sym == lss)
			tag = 1;//��վ<����
		else if (sym == lsq)
			tag = 2;
		else if (sym == rss)
			tag = 3;
		else if (sym == rsq)
			tag = 4;
		else if (sym == neq)
			tag = 5;
		else if (sym == eql)
			tag = 6;
		getsym();
		simple_exp(ptx);
		if (tag == 1)
			gen(opr, 0, 3);//��վ<����
		else if (tag == 2)
			gen(opr, 0, 4);
		else if (tag == 3)
			gen(opr, 0, 5);
		else if (tag == 4)
			gen(opr, 0, 6);
		else if (tag == 5)
			gen(opr, 0, 7);
		else if (tag == 6)
			gen(opr, 0, 8);
	}
}
void simple_exp(int* ptx) {
	term(ptx);
	while (sym == plus || sym == minus)
	{
		int tag = 0;
		if (sym == plus)
			tag = 1;
		getsym();
		term(ptx);
		if (tag == 1)
			gen(opr, 0, 9);
		else
			gen(opr, 0, 10);
	}
}
void term(int* ptx)
{
	factor(ptx);
	while (sym == times || sym == slash)
	{
		int tag = 0;
		if (sym == times)
			tag = 1;
		getsym();
		factor(ptx);
		if (tag == 1)
			gen(opr, 0, 11);//�˷� 
		else
			gen(opr, 0, 12);
	}
}
void factor(int* ptx)
{
	if (sym == lparen)
	{
		getsym();
		exp(ptx);
		if (sym == rparen)
		{
			getsym();
		}
	}
	else if (sym == number)
	{
		gen(lit, 0, num);
		getsym();
	}
	else if (sym == ident)
	{
		int i = position(id, *ptx);
		if (table[i].kind == 0)
		{
			gen(lod, 0, position(id, *ptx));
			getsym();
		}
		else if (table[i].kind == 1)
		{
			int x;
			getsym();
			if (sym == ls)
			{
				getsym();
				if (sym == number)
				{
					x = num;
					getsym();
					if (sym == rs)
					{
						//����һά����
						//ѹ�����ַ
						gen(getarr, 0, i);
						//ѹ��x
						gen(lit, 0, x);
						//�����ַ
						gen(opr, 0, 9);
						//���õ�ַ��������ջ��(load_arr)
						gen(load_arr, 0, 0);//��ջ����ָ���������ջ��
						getsym();
					}
					else
					{
						error = 8;
					}
				}
				else
				{
					error = 8;
				}
			}
			else
				error = 8;
		}
		else if (table[i].kind == 2)
		{
			int x, y;
			getsym();
			if (sym == ls)
			{
				getsym();
				if (sym == number)
				{
					x = num;
					getsym();
					if (sym == rs)
					{
						getsym();
						if (sym == ls)
						{
							getsym();
							if (sym == number)
							{
								y = num;
								getsym();
								if (sym == rs)
								{
									//�����ά����
									gen(getarr, 0, i);//��Ż�����ַ 
									gen(lit, 0, x);//��һ���±�
									gen(lit, 0, table[i].dix2);
									gen(opr, 0, 11);
									gen(opr, 0, 9);
									gen(lit, 0, y);
									gen(opr, 0, 9);
									gen(load_arr, 0, 0);
									getsym();
								}
								else
								{
									error = 8;
								}
							}
							else
							{
								error = 8;
							}
						}
						else
						{
							error = 8;
						}
					}
					else error = 8;
				}
				else error = 8;
			}
			else error = 8;
		}
		else error = 9;
	}
	else error = 5;
}
void interpret()
{
	int p = 0; /* ָ��ָ�� */
	int t = 0; /* ����ջ��ָ�� */
	struct instruction i;	/* ��ŵ�ǰָ�� */
	int s[stacksize];	/* ����ջ */
	printf("Start small\n");
	int x, y, z;
	int kind;
	int add;
	int dix1, dix2;
	//fprintf(fresult, "Start small\n");
	do
	{
		//ȡָ��
		i = code[p];
		p = p + 1;
		switch (i.f)
		{
		case lit:
			s[t] = i.a;
			t++;
			break;
		case lod:
			x = table[i.a].addr;//��������ֵ���
			s[t] = s[x];
			t++;
			break;
		case sto:
			x = table[i.a].addr;
			s[x] = s[t - 1];
			t = t - 1;
			break;
		case ini:
			add = table[i.a].addr;
			table[i.a].addr = t;
			t++;
			break;
		case jmp:
			p = i.a;
			break;
		case jpc:
			if (s[t - 1] == i.l)
			{
				p = i.a;
			}
			t = t - 1;
			break;
		case create_arr:
			kind = table[i.a].kind;
			if (kind == 1)
			{
				x = table[i.a].dix1;
				table[i.a].addr = t;
				t = t + x;
			}
			else if (kind == 2)
			{
				dix1 = table[i.a].dix1;
				dix2 = table[i.a].dix2;
				table[i.a].addr = t;
				t = t + dix1 * dix2;
			}
			break;
		case getarr://ȡ����ַ��ջ��
			s[t] = table[i.a].addr;
			t++;
			break;
		case load_arr:
			x = s[t - 1];//x��ʾ��ַ
			y = s[x];
			s[t - 1] = y;
			break;
		case store_arr://��վ��ѹ��ջ����ָ�ռ�
			x = s[t - 1];
			y = s[t - 2];
			s[x] = y;
			t = t - 2;
			break;
		case opr:
			switch (i.a) {
			case 1:
				//���ջ��Ԫ��
				printf("%d", s[t - 1]);
				fprintf(fresult, "%d", s[t - 1]);
				t = t - 1;
				break;
			case 2:
				//����ջ��Ԫ��
				x;
				scanf("%d", &x);
				s[t] = x;
				t++;
				break;
			case 3:
				//<
				x = s[t - 2];
				y = s[t - 1];
				z = (x < y);
				s[t - 2] = z;
				t--;
				break;
			case 4:
				x = s[t - 2];
				y = s[t - 1];
				z = (x <= y);
				s[t - 2] = z;
				t--;
				break;
			case 5:
				x = s[t - 2];
				y = s[t - 1];
				z = (x > y);
				s[t - 2] = z;
				t--;
				break;
			case 6:
				x = s[t - 2];
				y = s[t - 1];
				z = (x >= y);
				s[t - 2] = z;
				t--;
				break;
			case 7:
				x = s[t - 2];
				y = s[t - 1];
				z = (x != y);
				s[t - 2] = z;
				t--;
				break;
			case 8:
				x = s[t - 2];
				y = s[t - 1];
				z = (x == y);
				s[t - 2] = z;
				t--;
				break;
			case 9:
				x = s[t - 2];
				y = s[t - 1];
				z = (x + y);
				s[t - 2] = z;
				t--;
				break;
			case 10:
				x = s[t - 2];
				y = s[t - 1];
				z = (x - y);
				s[t - 2] = z;
				t--;
				break;
			case 11:
				x = s[t - 2];
				y = s[t - 1];
				z = (x * y);
				s[t - 2] = z;
				t--;
				break;
			case 12:
				x = s[t - 2];
				y = s[t - 1];
				z = (x / y);
				s[t - 2] = z;
				t--;
				break;

			}break;
		}
	} while (p <= cx);
	printf("End small\n");
	fprintf(fresult, "End small\n");
}