#ifndef MIPS_PREPARE_H
#define MIPS_PREPARE_H

#include <iostream>
#include <cstdio>
#include <map>
#include <vector>
#include <string>
#include <fstream>
#define M 10000000
//#define M 1000
#define NXT 16
using namespace std;

map<string, int> Label; //Label的映射

//总存储（数据、命令）
char MEMORY[M];
int REGISTER[34] = { 0 };
int MEM_LOW = 0;

//存储代码
vector<string> Code;

//读写
ifstream fin;
//ifstream fin("code.s");
//ofstream cout("code.out");
//ifstream cin("data.in");

//操作数映射
vector<string> forOperator = {
	"add", "addu", "addiu", "sub", "subu", "xor", "xoru", "rem", "remu", "seq", "sge", "sgt", "sle", "slt", "sne",
	"neg", "negu", "li", "move",
	"mfhi", "mflo",
	"jr", "jalr",
	"nop", "syscall",
	"mul", "mulu", "div", "divu",
	"b", "j", "jal",
	"beq", "bne", "bge", "ble", "bgt", "blt",
	"beqz", "bnez", "blez", "bgez", "bgtz", "bltz",
	"la", "lb", "lh", "lw",
	"sb", "sh", "sw"
};	

enum MIPS_OPERATOR {
	//First_scanf:
	ADD, ADDU, ADDIU, SUB, SUBU, XOR, XORU, REM, REMU, SEQ, SGE, SGT, SLE, SLT, SNE, // Rd R1 R2/Imm
	NEG, NEGU, LI, MOVE, //Rd R1/Imm
	MFHI, MFLO, //Rd
	JR, JALR, //R1
	NOP, SYSCALL,
	MUL, MULU, DIV, DIVU, //R1 R2/Imm
	//Second_scanf:
	B, J, JAL, //Label 
	BEQ, BNE, BGE, BLE, BGT, BLT, //R1 R2/Imm Label 
	BEQZ, BNEZ, BLEZ, BGEZ, BGTZ, BLTZ, //R1 Label 
	LA, LB, LH, LW, //Rd Address (Del) R1 -> Rd
	SB, SH, SW //R1 Address (Del) R1 -> Rd
};
map<string, int> OperatorIndex; //操作的映射

//寄存器映射
vector<string> forRegister = {
	"zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
	"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
	"s0", "s1", "s2" ,"s3", "s4", "s5", "s6", "s7",
	"t8", "t9", "k0", "k1",
	"gp", "sp", "fp", "ra", "lo", "hi", "EMPTY"
};
enum MIPS_REGISTER {
	ZERO, AT, V0, V1, A0, A1, A2, A3,
	T0, T1, T2, T3, T4, T5, T6, T7,
	S0, S1, S2, S3, S4, S5, S6, S7,
	T8, T9, K0, K1,
	GP, SP, FP, RA, LO, HI, EMPTY
};
const int S8 = 30;
map<string, int> RegisterIndex; //寄存器的映射

								//命令类
class OperatorCode { //命令存储类 
public:
	char operatorIndex; //操作编号
						//存放寄存器，操作数1，操作数2
	char Register[3];
	int Imm, Label, Delta;
	OperatorCode(char index = 255) :operatorIndex(index) {
		for (int i = 0; i < 3; ++i) Register[i] = EMPTY;
		Delta = 0;
	}
	void load(int loc) {
		operatorIndex = MEMORY[loc++];
		for (int i = 0; i < 3; ++i) {
			Register[i] = MEMORY[loc++];
		}
		Imm = *((int*)(MEMORY + loc));
		Label = *((int*)(MEMORY + loc + 4));
		Delta = *((int*)(MEMORY + loc + 8));
	}
};

//########################################  转换  ################################################

int toInt(string & line, int & loc, int len); //string中从loc提取数字

int toRInt(string & line, int & loc, int len); //string中从loc提取寄存器编号

void saveInt(int x, int & loc, int ord); //将int塞入char数组

void saveString(string & str, int & loc); //将string塞入char数组

void saveStruct(string & line, int & loc, int len, int l, int r, int index, bool label); //从loc位置构造命令塞入char数组

//################################################################################################

//#######################################  预处理  ###############################################

void MIPS_PRETREATMENT(); //预处理

void code_in();

void prepare(); //建立寄存器和操作的map映射

void code_scanf();

//void second_scanf();

void manageString(string & line, int & loc, int & len); //处理空行，跳过前置空格

//################################################################################################


int MIPS_SIMULATION();

void Debug(OperatorCode & _OperatorCode, int & cnt, int REGISTER[]) {
	cout << "Debug: \n{\n" << ++cnt << "\nOperator: " << forOperator[_OperatorCode.operatorIndex] << '\n';
	for (int i = 0; i < 3; ++i)
		cout << "Register[" << i << "]: " << int(_OperatorCode.Register[i]) << " or " << forRegister[_OperatorCode.Register[i]] << '\n';
	cout << "Delta: " << _OperatorCode.Delta << '\n'
		<< "Imm :" << _OperatorCode.Imm << '\n'
		<< "Label :" << _OperatorCode.Label << "\n";
	for (int i = 0; i < 34; ++i) cout << i << '\t'; cout << '\n';
	for (int i = 0; i < 34; ++i) cout << REGISTER[i] << '\t'; cout << "\n}\n\n";
}

#endif // !MIPS_PREPARE
