/*
	Game Compiler for 8080	(M80.COM Version)
	Ver.	0.0
	** Copyright (c) by S.Takeoka **

	31/AUG/2011 (8080 under Linux)

	31/Jul/1986	(8080 on SUN)
	31/Jan/1986	(8080)
	02/Aug/1985	(8080)
	17/Feb/1985	(Z80)
	14/Jan/1984	(original MB-6890 Version 4.21)

	This compiler needs "game80.GGG" as runtime routine package.

*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>


FILE *sf,*of;
char sfn[20],ofn[20];


int l[1000];		/* l L (Label)*/
int lp;			/* lp M (LabelMaxPointer)*/
char y[256];		/* y Y (GAME text buf)*/
int otop,olast;		/* otop L[1] , olast L[-1] */
int lno;		/* lno Q (current LineNo.)*/
int oadrs;		/* oadrs A (ObjectAddress)*/
char *G,*GSAVE;		/* GAME text  G */
char pass;		/* pass P  */
int ifc;char iff;	/* if line	*/
char numflg;		/* num ?	*/
char nest;		/* nesting	*/

main(int argc,char *argv[])
{
 puts("GAME 8080 Compiler (M80)V.0.0\nCopyright (c) by S.Takeoka\n");
 fname(argc,argv);
 compile();
/* printf("\n$%04x to $%04x\007",otop,olast); */
}

fname(int argc,char *argv[])
{switch(argc){
   case 1 :	puts("Source Name?  ");gets(sfn);
		puts("Object Name?  ");gets(ofn);break;
   case 2 :	strcpy(sfn,argv[1]);strcat(sfn,".g");
		strcpy(ofn,argv[1]);strcat(ofn,".mac");break;
   default:	strcpy(sfn,argv[1]);
		strcpy(ofn,argv[2]);break;
 }
}

take(int c)
{printf("  %04x",c);return c;}

compile()
{
 if(NULL==(sf=fopen(sfn,"r"))){printf("File %s cannot open\n",sfn);exit(1);}
 pass=0;lp=0;oadrs=otop;ifc=0;nest=0;
 cmpl1();
 fclose(sf);
 l[lp++]=32767;
 if(nest)xerror("Nest");
 pass=1;     oadrs=otop;ifc=0;nest=0;
 sf=fopen(sfn,"r"); of=fopen(ofn,"w");
 runtime();
 cmpl1();
 fclose(of); fclose(sf);
}


getg() { return *G++; }

ungetg(char c) { *(--G)=c;}

cmpl1()
{int i;char *bp;
 printf("Pass %d ",pass+1);
 for(iff=0;;){
   /* -Top of line- */
   if(iff){obj("I",0);objnum1(ifc++,0);obj(":\n",0);iff=0;}
   bp=y;

   if(NULL==fgets(y,256,sf)) break;
   i=strlen(y);y[i-1]='\0';
   G=y;
 obj(";",0);obj(G,0);obj("\n",0);
   skpspc();
   lno=getint();
   if(pass && srclno(lno)){obj("L",0);objnum1(lno,0);obj(":",0);}
   switch(getg()){
	case 'c' : codenot(); break;
	case 'd' : datanot(); break;
	case ' ' : cmd1line(); break;
	default  : comment(); break;
   }
 }
 obj("JMP EXIT\n;DSEG\nOLAST:\nEND\n",3);
obj(
";GAME 8080 Compiler (M80)(V0.0 02/Aug/1985) by S.Takeoka under CP/M 2.2\n"
,0);
 xputs("end\n");
}

codenot() /* Code Notation */
{char c;int n;
 do{
   n=getconst();
   c=getg();if(c & 0xDF){ xerror("code"); }
   obj("DB ",0);objnum(n,1);
 }while(c!=0);
}

datanot() /* Data Notation */
{char c;int n;
 do{
   n=getconst();
   c=getg(); if(c & 0xDF){ xerror("data"); }
   obj("DW ",0);objnum(n,2);
 }while(c!=0);
}

getconst()/* 6500 -Get Const*/
{char c;
 skpspc();
 if((c=getg())=='$') return gethex();
 if(c=='!'){linenum(); return;}
 ungetg(c);
 return getint();
}

comment() { while(getg()); }

cmd1line() /* Command 1 Line */
{char c;
 for(;;){
   /* -Top of Command- */
   skpspc();
   c=getg();GSAVE=G;
   switch(c){
	case  0  : return;	/*end of line */
	case '/' : obj("CALL CRLF\n",3);continue;	/*CRLF*/
	case ']' : obj("RET\n",1);comment();return;	/*RETURN*/
 	case '#' : go_(1);comment();return;		/*GOTO*/
	case '!' : go_(0);continue;			/*GOSUB*/
	case '@' : dountil();continue;			/*DO/UNTIL*/
	case '"' : prt();continue;			/*PRSTR*/
	case '\\': xerror("MB-6890 Opcom");
	default  :;
   }
   /* Cmd=Exp  */
   skp1('=');getg();/* aboid'='*/
   expr();
   switch(c){
	case '$' : prchar();continue;
	case '?' : prnum();continue;
	case '.' : obj("CALL SPC\n",3);continue;
 	case '>' : obj("LXI B,$+5\nPUSH B\nPCHL\nSHLD MRET\n",8);continue;
	case ';' : obj("MOV A,H\nORA L\nJZ I",3);objnum(ifc,0);iff=1;continue;
	default  : if(strvar(c))continue;
			/*else xerror("undef'd Cmd");*/
   }
 }
}

dountil()
{char c,str[100];
 int vname;
 if((c=getg())!='='){	/* do */
   ungetg(c);
   obj("LXI B,$+8\nPUSH B\nLXI  H,0\nPUSH H\n",6);
 }else{			/* until */
   vname=getg();ungetg(vname);
   expr();
   strcpy(str,"SHLD ");varadrs(str,vname);strcat(str,"\n");
   obj(str,3);
   obj("POP B\nSTC\nMOV A,L\nSBB C\nMOV A,H\nSBB B\n",0);
   obj("POP H\nJP $+6\nPUSH H\nPUSH B\nPCHL\n",13);
 }
}

prt()	/* print const str */
{char c;
 obj("CALL PRSTR\n",3);
 while((c=getg())!='"'){
   obj("DB ",0);objnum(c,1);
 }
 obj("DB 0\n",1);
}

prchar()/* see Line 300 I H */
{char c,*sv;
 sv=G; G=GSAVE;
 if((c=getg())==':'){/*FilePutc*/
   lvlsubexpr();obj("CALL FPUTC\n",3);objpop();
 }else{/*TerminalPutchar*/
   ungetg(c);
   obj("CALL PUTC\n",3);
 }
 G=sv;
}

lvlsubexpr()/*LeftValueSubExpression*/
{
 objpush();/*pushRightVALUE*/
 texpr1();
}

prnum()
{char c,*sv;
 sv=G;G=GSAVE;
 switch(c=getg()){
	case '?' : obj("CALL PR4HEX\n",3); break;
	case '$' : obj("CALL PR2HEX\n",3); break;
	case '(' : lvlsubexpr();obj("CALL PRDECR\n",3);objpop();break;
	default  : obj("CALL PRDEC\n",3);
 }
 G=sv;
}

/* 1900--If Line-- ifline(){lineadrs(lno);} */

go_(int x)
{if(getg()!='=') xerror("# or !");
 if(x){ obj("JMP " ,1);linenum();}	/*goto # */
 else{  obj("CALL ",1);linenum();}	/*gosub ! */
}

/* 2000--Line number--*/
linenum()
{char c;
 if((c=getg())=='-'){obj("EXIT\n",2);getint();if(!numflg)xerror("-LineNum");}
 else{ungetg(c);obj("L",0);getlno();}
}

getlno()
{char c;int ln;
 objnum(ln=getint(),2);
 if(!pass){ l[lp++]=ln;}
 c=getg();
 if((!numflg)||(c & 0xDF))xerror("LineNum");
 ungetg(c);
 return ;
}

srclno(int ln)
{int i;
 for(i=0;i<lp;i++) if(ln==l[i])return 1;
 return 0;
}

/*  4003--Expression  4010 */
expr()
{char o;
 term();
 for(;;){
   switch(o=getg()){
	case '+' : term1();obj("DAD D\n",2);objpop();continue;
	case '-' : term1();
		obj("MOV A,E\nSUB L\nMOV L,A\nMOV A,D\nSBB H\nMOV H,A\n",6);
		objpop();continue;
	case '*' : term1();obj("CALL MULT\n",4);objpop();continue;
	case '/' : term1();obj("CALL DIV\n",4);objpop();continue;
	case '.' : term1();
	obj("MOV A,L\nORA E\nMOV L,A\nMOV A,H\nORA D\nMOV H,A\n",7);objpop();
		continue;
	case '&' : term1();
	obj("MOV A,L\nANA E\nMOV L,A\nMOV A,H\nANA D\nMOV H,A\n",7);objpop();
		continue;
	case '!' : term1();
	obj("MOV A,L\nXRA E\nMOV L,A\nMOV A,H\nXRA D\nMOV H,A\n",7);objpop();
		continue;
	case '=' : term1();
	obj("MOV A,L\nSUB E\nMOV L,A\nMOV A,H\nSBB D\nORA L\n",6);
	objpop();obj("LXI H,0\nJNZ $+4\nINR L\n",7);
		continue;
	case '<' : less();continue;
	case '>' : greater();continue;
	default  : ungetg(o); return;
   }
 }
}

less()
{char o;
 switch(o=getg()){
	case '>' :/* not eq */
		term1();
		obj(
			"MOV A,L\nSUB E\nMOV L,A\nMOV A,H\nSBB D\nORA L\n"
		,6);objpop();
		obj("LXI H,0\nJZ $+4\nINR L\n",7);
		break;
	case '=' :/* less eq */
		term1();obj("CALL LEQ\n",3);objpop();break;
	default  :/* less */
		ungetg(o);
		term1();obj("CALL LES\n",3);objpop();
 }
}

greater()
{char o;
 switch(o=getg()){
	case '=' :/* greater eq */
		term1();obj("CALL GEQ\n",3);objpop();break;
	default  :/* greater */
		ungetg(o);
		term1();obj("CALL GRE\n",3);objpop();
 }
}

term1()/* term with pushing 5000*/
{objpush();
 term();
}

/* 5005--TERM-- 5010 */
term()
{
 if(func())return;
 else if(texpr())return;
 else if(xfunc_const())return;
 else if(ldvar())return;
}

texpr()
{char c;
 c=getg(); if(c!='('){ ungetg(c);return 0;}
 texpr1();
 return 1;
}

texpr1()
{ expr();if(getg()!=')') xerror("expr )");
}

xfunc_const()/* SpecialFunction & Const 5007 */
{char c;int n;
 switch((c=getg())){
	case '$' : inchar_hex();break;
	case '?' : obj("CALL INPNUM\n",3);break;
	case '"' : obj("LXI H,",1);objnum(getg(),2);getg();break;
	case '!' : obj("LXI H,",1);linenum();break;
	case '&' : obj("LXI H,OLAST\n",3);break;
	case '<' : obj("CALL KEY\n",3);break;
	default  : ungetg(c);n=getint();
		if(numflg){obj("LXI H,",1);objnum(n,2);break;}
			return 0;
 }
 return 1;
}

inchar_hex()
{char c;
 c=getg();
 if(c==':'){/*FileGetc*/
   texpr1();obj("CALL FGETC\n",3);
 }else{/*GetKey*/
   ungetg(c);
   if(ishex(c)){ obj("LXI H,",1);objnum(gethex(),2);return;/*constHEX*/}
   obj("CALL GETC\n",3);
 }
}

func()/* 5079-Func 5080 */
{char c;
 switch(c=getg()){
	case '+' : term();obj("MOV A,H\nORA A\nCM NEGAT\n",5);break;
	case '-' : term();obj("CALL NEGAT\n",3);break;
	case '#' :
		term();obj("MOV A,H\nORA L\nJZ $+6\nLXI H,00FFH\nINR L\n",9);
		break;
	case '\'': term();obj("CALL RAND\n",3);break;
	case '%' : term();obj("LHLD MODUL\n",3);break;
	default  : ungetg(c);return 0;
 }
 return 1;
}

/* Variables */
strvar(int vname)/* 500 -Store Variable*/
{char c,*sv,str[100];
 sv=G;G=GSAVE;
 switch(c=skpvar()){
	case '(' :/* v(x) */
		strv1(vname,1);obj("MOV M,E\nINX H\nMOV M,D\n",3);G=sv;return;
	case ':' :/* v:x) */
		strv1(vname,0);obj("MOV M,E\n",1);G=sv;return;
	default  :
		strcpy(str,"SHLD ");varadrs(str,vname);strcat(str,"\n");
		obj(str,3);
		G=sv;
		if((c=getg())==','){/* v=expr,expr */
			expr();
			obj("LXI B,$+5\nPUSH B\nPUSH H\n",6);
			return;
		}else{/* v */
			ungetg(c);/*if(c!=' ')xerror("StrVarExp");*/
		}
 }
}

strv1(int vname,int dbl)
{char str[100];
 objpush();
 texpr1();
 if(dbl){obj("DAD H\n",1);}/* <<1 */
 strcpy(str,"LDA ");varadrs(str,vname);strcat(str,"\n");obj(str,3);
 obj("MOV C,A\n",1);
 strcpy(str,"LDA ");varadrs(str,vname);strcat(str,"+1\n");obj(str,3);
 obj("MOV B,A\n",1);
 obj("DAD B\n",1);
 objpop();
}


ldvar()/* 5200-Load Variable */
{char c,str[100];
 int vname;
 vname=getg();if(isdigit(vname)){ungetg(vname);return 0;}
 c=skpvar();
 switch(c){
	case '(' :/* v(x) */
		ldvar1(vname,1);
		obj("MOV E,M\nINX H\nMOV D,M\nXCHG\n",4);objpop();break;
	case ':' :/* v:x) */
		ldvar1(vname,0);
		obj("MOV L,M\nMVI H,0\n",3);objpop();break;
	default  :/* v */
		ungetg(c);
		strcpy(str,"LHLD ");varadrs(str,vname);strcat(str,"\n");
		obj(str,3);
 }
 return 1;
}

ldvar1(int vname,int dbl)/* 5600 */
{char str[100];
 strcpy(str,"LHLD ");varadrs(str,vname);strcat(str,"\n");obj(str,3);
 objpush();
 texpr1();
 if(dbl){obj("DAD H\n",1);}/* <<1 */
 obj("DAD D\n",1);
}

/* -Var Subs- */
skpvar()/* 5500 */
{char c;
 while((c=getg())>='A');
 return c;
}

varadrs(char *str, int vname)
{char *s;
 for(s=str;*s++;);s--;
 if(vname>='A'){ *s++='V';*s++ =vname;}
 else switch(vname){
	case '\'': *s++='R';*s++='N';*s++='D';break;
	case '*' : *s++='R';*s++='A';*s++='M';*s++='E';*s++='N';*s++='D';break;
	case '>' : *s++='M';*s++='R';*s++='E';*s++='T';break;
	default  : xerror("Variable Name");
 }
 *s='\0';
}

getint()/* Get INT 6000 */
{int n;char c;
 for(numflg= n =0; isdigit(c=getg());){
   n= n*10+ c-'0'; numflg=1;
 }
 ungetg(c);
 return n;
}

gethex()/* -Get HEX 6050 */
{int n;char c;
 for(numflg=n=0;ishex(c=getg());){
   n= ( n << 4) + ( c<'A' ? c-'0' : c-'A'+10 ); numflg=1;
 }
 ungetg(c);
 return n;
}

ishex(int c)
{ return ('0'<=c && c<='9') || ('A'<=c && c<='F') ; }


/* 6100 -Skip SPC- */
skpspc()
{char c;
 while((c=getg())==' ');
 ungetg(c);
}


skp1(int cc)
{char c;
 while((c=getg())!=cc){ if((c & 0xDF)==0) xerror("Syntax"); }
 ungetg(c);
}


obj(char *str,int bytes)
{
 if(pass) fputs(str,of);
 oadrs+=bytes;
}

objnum(int n,int bytes)
{ objnum1(n,bytes);if(pass)fputs("\n",of); }

objnum1(int n,int bytes)
{char str[10],*s,x;
 str[9]='\0'; str[8]='H'; s= &str[7];
 do{
   x= n & 0xF ;*s-- = x<10 ? x+'0' : x+'A'-10; n >>= 4;
 }while(n);
 *s='0';
 if(pass){fputs( s,of );}
 oadrs+=bytes;
}

objpop()
{
 if(--nest) obj("POP D\n",1);
}

objpush()
{
 if(nest++) obj("PUSH D\n",1);
 obj("XCHG\n",1);
}

/* 20000 */
xerror(char *s)
{char c;
 xputs(s);
 printf(" ERR in Line %d",lno); allclose();exit(1);
}

xputs(char *s)
{ char c;while(c= *s++)putc(c,stdout); }

allclose(){fclose(sf);fclose(of);}

#ifndef MACRO80
#ifndef BDS_C
runtime()
{
 int c;
 FILE *f;
 f=fopen("game80.GGG","r");
 if(f ==NULL) xerror("runtime routines(game80.GGG) not found!");
 while((c=getc(f))!=EOF)
	putc(c,of);
 fclose(f);
}
#else BDS_C
runtime()
{
 char c;FILE f;
 fopen("GAME80.GGG",&f);
 while((c=getc(&f))!=CPMEOF) putc(c,of);
 fclose(&f);
}
#endif
#else /*MACRO 80 */
runtime()
{/*
 char c;FILE f;
 fopen("GAME80.GGG",&f);
 while((c=getc(&f))!=CPMEOF) putc(c,of);
 close(&f);
 */
 obj(";",0);obj(ofn,0);obj("\n",0);
 obj("extrn	CRLF,	SPC,	PRSTR,	FPUTC,	PUTC,	PR4HEX,	PR2HEX\n",0);
 obj("extrn	PRDECR,	PRDEC,	MULT,	DIV,	LEQ,	LES,	GEQ\n",0);
 obj("extrn	GRE,	INPNUM,	FGETC,	GETC,	NEGAT,	RAND\n",0);
 obj("extrn	MODUL,	RND,	RAMEND,	MRET\n",0);
 obj("extrn	EXIT,	KEY\n",0);
 obj("extrn	va,vb,vc,vd,ve,vf,vg,vh,vi,vj,vk,vl,vm,vn,vo\n",0);
 obj("extrn	vp,vq,vr,vs,vt,vu,vv,vw,vx,vy,vz\n",0);
 obj("entry	MAIN\n",0);
 obj(";CSEG\n",0);
 obj("MAIN:\n",0);
}
#endif

/*
	GAME80.GGG
	-Sub Pack-
extrn	CRLF,	SPC,	PRSTR,	FPUTC,	PUTC,	PR4HEX,	PR2HEX
extrn	PRDECR,	PRDEC,	MULT,	DIV,	LEQ,	LES,	GEQ
extrn	GRE,	INPNUM,	FGETC,	GETC,	NEGAT,	RAND

extrn	MODUL,	RND,	RAMEND,	MRET
extrn	EXIT,	KEY
entry	MAIN
*/

