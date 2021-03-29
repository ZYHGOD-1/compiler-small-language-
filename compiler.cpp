#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#define norw 12 //保留字个数
#define id_max 15//标识符最大的长度 
#define number_max 14//数字最大的长度 
#define table_max 100//符号表容量 
#define cxmax 500//虚拟机代码最大数
#define amax 2048     /* 地址上界*/
#define stacksize 5000 //数据栈最大长度

enum symbol {
	nul, ident, number, plus, minus, times, slash, lparen, rparen, ls, rs, eql, neq, rss, rsq, lsq, lss, semicolon, becoms, beginsym, endsym,
	repeatsym, readsym, writesym, assignmentsym, thensym, ifsym, untilsym, elsesym, arrsym
};
struct tablestruct {
	char name[id_max];
	int addr;//变量的地址或者数组的地址
	int kind = 0;//0表示变量，1表示一维数组，2表示2维数组
	int dix1 = 0;
	int dix2 = 0;
}; //符号表结构


/* 虚拟机代码指令 */
enum fct {
	lit, opr, lod,
	sto, cmp, ini,
	jmp, jpc, getarr, create_arr, store_arr, load_arr//取数组 
};


#define fctnum 13
/* 虚拟机代码结构 */
struct instruction
{
	enum fct f; /* 虚拟机代码指令 */
	int l;      /* 引用层与声明层的层次差 */
	int a;      /* 根据f的不同而不同 */
};



struct tablestruct table[table_max];//符号表 
char ch;//用于getch
enum symbol sym;//当前符号
int cx;             /* 虚拟机代码指针, 取值范围[0, cxmax-1]*/
int cc, ll;         //getch使用的计数器，cc表示当前字符(ch)的位置 
char line[81];      /* 读取行缓冲区 */
char word[norw][id_max];        /* 保留字 */
enum symbol wsym[norw];     /* 保留字对应的符号值 */
enum symbol ssym[256];      /* 单字符的符号值 */
char a[id_max + 1];//临时符号
char id[id_max + 1];
int num;//当前数字 
struct instruction code[cxmax]; /* 存放虚拟机代码的数组 */
char mnemonic[fctnum][20];   /* 虚拟机代码指令名称 */

FILE* fin;
FILE* fcode;
FILE* fresult;

int error = -1;//错误处理,1表示数字位数太多，2表示符号表满,3表示未识别的保留字
		 //4表示变量没有申明,5表示无法识别


void init();
void getch();
void getsym();
//ptx指向表尾 
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
int position(char* id_, int tx)//查找标识符在表中的位置,-1时出错
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
		printf("Program is too long!\n");	/* 生成的虚拟机代码程序过长 */
		exit(1);
	}
	if (z >= amax)
	{
		printf("Displacement address is too big!\n");	/* 地址偏移越界 */
		exit(1);
	}
	code[cx].f = x;
	code[cx].l = y;
	code[cx].a = z;
	cx++;
}
void interpret();

//初始化
void init()
{
	//设置单字符 
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
	//设置保留字名称
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
	//设置保留字符号
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
	//指令集
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
	if (cc == ll) /* 判断缓冲区中是否有字符，若无字符，则读入下一行字符到缓冲区中 */
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
void getsym()//词法分析 
{
	int i, j, k;
	while (ch == ' ' || ch == 10 || ch == 9)	/* 过滤空格、换行和制表符 */
	{
		getch();
	}
	if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) /* 当前的单词是标识符或是保留字 */
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
		if (tag == 1) /* 当前的单词是保留字 */
		{
			sym = wsym[i];
			//printf("是保留字\n");
		}
		else /* 当前的单词是标识符 */
		{
			sym = ident;
			//printf("是标识符 %d\n",(sym==ident));
		}
	}
	else
	{
		if (ch >= '0' && ch <= '9') /* 当前的单词是数字 */
		{
			k = 0;
			num = 0;
			sym = number;
			do {
				num = 10 * num + ch - '0';
				k++;
				getch();;
			} while (ch >= '0' && ch <= '9'); /* 获取数字的值 */
			k--;
			//printf("数字的值：%d\n",num);
			if (k > number_max) /* 数字位数太多 */
			{
				error = 1;//数字位数太多
			}
		}
		else
		{
			if (ch == ':')		/* 检测赋值符号 */
			{
				getch();
				if (ch == '=')
				{
					sym = becoms;
					//printf("是赋值号\n");
					getch();
				}
				else
				{
					sym = nul;	/* 不能识别的符号 */
				}
			}
			else
			{
				if (ch == '<')		/* 检测小于或小于等于符号 */
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
					if (ch == '>')		/* 检测大于或大于等于符号 */
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
						//printf("是单字符\n");	/* 当符号不满足上述条件时，全部按照单字符符号处理 */                   
					}
				}
			}
		}
	}
}
void enter(int* ptx, int add)//将当前的符号加入符号表 
{
	(*ptx)++;
	if (*ptx >= table_max)
		error = 2;//符号表满
	else
	{
		strcpy(table[(*ptx)].name, id);
		table[(*ptx)].addr = add;
	} /* 符号表的name域记录标识符的名字 */
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
	if (sym == ifsym)//if---else 语句
	{
		getsym();
		exp(ptx);//翻译表达式
		int now1 = cx;
		gen(jpc, 0, -1);//不满足就跳转，-1表示待回填
		if (sym == thensym)
		{
			getsym();
			stmt_sequence(ptx);
			int now2 = cx;
			gen(jmp, 0, -1);//无条件跳转指令,待回填
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
	else if (sym == repeatsym)//repeat语句
	{
		getsym();
		int now1 = cx;
		gen(jmp, 0, -1);//无条件跳转指令，待回填
		stmt_sequence(ptx);
		code[now1].a = cx;//回填
		if (sym == untilsym)
		{
			getsym();
			exp(ptx);
			gen(jpc, 0, now1 + 1);//不成立就跳转
		}
	}
	else if (sym == ident)//assign语句
	{

		int i = position(id, *ptx);
		if (i == -1)//不在符号表中 
		{
			enter(ptx, -1);
			i = position(id, *ptx);
			table[i].kind = 0;
			gen(ini, 0, i);
		}//加入符号表,地址初始化-1
		i = position(id, *ptx);
		if (table[i].kind == 0)//是变量
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
										//二维数组赋值操作
										//处理exp
										getsym();
										exp(ptx);
										//基地址入栈（getarr）
										gen(getarr, 0, i);
										//x入栈
										gen(lit, 0, x);
										//size入栈
										gen(lit, 0, table[i].dix2);
										//计算地址
										gen(opr, 0, 11);
										gen(opr, 0, 9);
										//y入栈
										gen(lit, 0, y);
										gen(opr, 0, 9);

										//存放（store_arr）
										gen(store_arr, 0, 0);//为0表示将次站顶压入栈顶所指向的空间
									}
								}
								else error = 7;
							}
							else
								error = 7;
						}
						else if (sym == becoms)
						{
							//一维数组赋值操作
							//处理exp
							getsym();
							exp(ptx);
							//基地址入栈（getarr）
							gen(getarr, 0, i);
							//x入栈
							gen(lit, 0, x);
							//size入栈
							//计算地址
							gen(opr, 0, 9);

							//存放（store_arr）
							gen(store_arr, 0, 0);//为0表示将次站顶压入栈顶所指向的空间
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
	else if (sym == writesym)//write语句
	{
		getsym();
		exp(ptx);
		gen(opr, 0, 1);//1表示输出

	}
	else if (sym == readsym)//read语句
	{
		getsym();
		if (sym == ident)
		{
			int i = position(id, *ptx);//要读的变量的位置
			if (i == -1)
				error = 4;//变量没有申明
			if (i != -1)
			{
				if (table[i].kind == 0)
				{
					gen(lod, 0, i);//将变量加载到数据栈
					gen(opr, 0, 2);
				}//2表示输出栈顶元素
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
								//处理一维数组
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
											//处理二维数组
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
	else if (sym == arrsym)//数组声明变量
	{
		getsym();
		enter(ptx, -1);//插入数组名到符号表
		int i = position(id, *ptx);//得到位置
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
								//二维数组
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
						//一维数组
						table[i].dix1 = dix1;
						table[i].kind = 1;
						gen(create_arr, 0, i);//开辟空间 
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
			error = 6;//数组声明错误
		}
	}
	else { error = 3; }//未识别的保留字
}
void exp(int* ptx)
{
	simple_exp(ptx);
	if (sym == lss || sym == lsq || sym == rss || sym == rsq || sym == neq || sym == eql)
	{
		int tag = 0;
		if (sym == lss)
			tag = 1;//次站<顶？
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
			gen(opr, 0, 3);//次站<顶？
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
			gen(opr, 0, 11);//乘法 
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
						//处理一维数组
						//压入基地址
						gen(getarr, 0, i);
						//压入x
						gen(lit, 0, x);
						//计算地址
						gen(opr, 0, 9);
						//将该地址的数读入栈顶(load_arr)
						gen(load_arr, 0, 0);//将栈顶所指向的数读入栈顶
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
									//处理二维数组
									gen(getarr, 0, i);//存放基本地址 
									gen(lit, 0, x);//第一个下标
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
	int p = 0; /* 指令指针 */
	int t = 0; /* 数据栈顶指针 */
	struct instruction i;	/* 存放当前指令 */
	int s[stacksize];	/* 数据栈 */
	printf("Start small\n");
	int x, y, z;
	int kind;
	int add;
	int dix1, dix2;
	//fprintf(fresult, "Start small\n");
	do
	{
		//取指令
		i = code[p];
		p = p + 1;
		switch (i.f)
		{
		case lit:
			s[t] = i.a;
			t++;
			break;
		case lod:
			x = table[i.a].addr;//操作数的值提出
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
		case getarr://取基地址放栈顶
			s[t] = table[i.a].addr;
			t++;
			break;
		case load_arr:
			x = s[t - 1];//x表示地址
			y = s[x];
			s[t - 1] = y;
			break;
		case store_arr://次站顶压入栈顶所指空间
			x = s[t - 1];
			y = s[t - 2];
			s[x] = y;
			t = t - 2;
			break;
		case opr:
			switch (i.a) {
			case 1:
				//输出栈顶元素
				printf("%d", s[t - 1]);
				fprintf(fresult, "%d", s[t - 1]);
				t = t - 1;
				break;
			case 2:
				//输入栈顶元素
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