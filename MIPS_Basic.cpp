#include "MIPS_Basic.h"

int main(int argc, char *argv[])
//int main()
{
	fin = ifstream(argv[1]);
	MIPS_PRETREATMENT();
	/*for (int i = 0; i < MEM_LOW; ++i) {
		cout << "line " << i << ": { " << int(MEMORY[i]) << " or " << MEMORY[i] << " }\n";
	}*/
	return MIPS_SIMULATION();
}

//########################################  转换  ################################################

int toInt(string & line, int & loc, int len) {
	int ret = 0, f = 1;
	for (; !(line[loc] == '-' || (line[loc] >= '0' && line[loc] <= '9')); ++loc);
	if (line[loc] == '-') f = -1, ++loc;
	for (; loc < len && line[loc] >= '0' && line[loc] <= '9'; ret = ret * 10 + line[loc] - '0', ++loc);
	// cout << "Debug " << ret * f << '\n';
	return ret * f;
}

int toRInt(string & line, int &loc, int len) {
	for (; line[loc] != '$'; ++loc);
	++loc;
	if (line[loc] >= '0' && line[loc] <= '9') {
		int ret = 0;
		for (; line[loc] >= '0' && line[loc] <= '9'; ret = ret * 10 + line[loc] - '0', ++loc);
		// cout << "Debug " << ret << '\n';
		return ret;
	}
	else {
		int nxt = loc;
		for (; line[nxt] != ' ' && line[nxt] != '\t' && line[nxt] != '\n' && line[nxt] != ',' && line[nxt] != ')'; ++nxt);
		string RegisterName(&line[loc], &line[nxt]); loc = nxt;
		// cout << "Debug " << RegisterName << ' ' << RegisterIndex[RegisterName] << '\n';
		return RegisterIndex[RegisterName];
	}
}

void saveInt(int x, int & loc, int ord) {
	if (ord == 1) new(MEMORY + loc) char(x);
	else if (ord==2) new(MEMORY + loc) short(x);
	else new(MEMORY + loc) int(x);
	loc += ord;
}

void saveString(string & str, int & loc) {
	str.push_back('\0');
	for (int len = str.length(), i = 0; i < len - 1; ++i) {
		char chr = str[i];
		if (str[i] == '\\') {
			++i;
			switch (str[i])
			{
			case('a'): chr = 7; break;
			case('b'): chr = 8; break;
			case('f'): chr = 12; break;
			case('n'): chr = 10; break;
			case('r'): chr = 13; break;
			case('t'): chr = 9; break;
			case('v'): chr = 11; break;
			case('\\'): chr = 92; break;
			case('\''): chr = 39; break;
			case('\"'): chr = 34; break;
			case('\?'): chr = 63; break;
			case('0'): chr = 0; break;
			default: --i; break;
			}
		}
		MEMORY[loc++] = chr;
	}
	//MEMORY[loc++] = '\0';
}

void saveStruct(string & line, int & loc, int len, int l, int r, int index, bool label) {
	OperatorCode tmp(index); int i = l;
	for (; i <= r; ++i) {
		//for (; loc < len && !((line[loc] >= '0' && line[loc] <= '9') || line[loc] == '-' || line[loc] == '$'); ++loc);
		for (; loc < len && (line[loc] == ' ' || line[loc] == ',' || line[loc] == '\t' || line[loc] == '\n'); ++loc);
		if (loc == len) break;
		if (line[loc] == '$') tmp.Register[i] = toRInt(line, loc, len);
		else tmp.Imm = toInt(line, loc, len);
	}
	tmp.Delta = i - l;
	if (label) {
		for (; line[loc] == ' ' || line[loc] == '\t' || line[loc] == ','; ++loc);
		if (line[loc] == '-' || (line[loc] >= '0' && line[loc] <= '9')) {
			tmp.Delta = toInt(line, loc, len);
			++loc;
			//处理 L型 S型命令
			if (tmp.Register[0] == EMPTY) tmp.Register[0] = toRInt(line, loc, len);
			else tmp.Register[1] = toRInt(line, loc, len);
		}
		else {
			int nxt = loc;
			for (; line[nxt] != ' ' && line[nxt] != '\t' && line[nxt] != '\n'; ++nxt);
			string labelName(&line[loc], &line[nxt]);
			tmp.Label = Label[labelName];
			// cout << "Debug " << labelName << ' ' << tmp.Label << '\n';
		}
	}
	new(MEMORY + MEM_LOW) OperatorCode(tmp);
	MEM_LOW += NXT;
	// cout << "Debug ML = " << MEM_LOW << '\n';
}

//################################################################################################

//#######################################  预处理  ###############################################

void MIPS_PRETREATMENT() {
	prepare();
	code_in();
	code_scanf();
	MEM_LOW = 0;
	code_scanf();
	REGISTER[SP] = M - 1;
}

void prepare() {
	for (int i = 0; i < forOperator.size(); ++i) OperatorIndex[forOperator[i]] = i;
	for (int i = 0; i < forRegister.size(); ++i) RegisterIndex[forRegister[i]] = i;
	RegisterIndex["s8"] = 30;
}

void code_in() {
	string line;
	while (getline(fin, line)) line.push_back('\n'), Code.push_back(line);
}

void code_scanf() {
	string line;
	for (int i = 0; i < Code.size(); ++i) {
		line = Code[i];
		int len = line.length(), loc = 0, annotation = line.find('#');
		if (annotation != -1) {
			line.erase(annotation, len - annotation);
			line.push_back('\n');
			len = annotation + 1;
		}
		for (; loc < len && (line[loc] == ' ' || line[loc] == '\t' || line[loc] == '\n'); ++loc); // "  .data" -> loc: "  ^data"
		if (loc == len) continue;
		int pos = line.find(':'), ept = line.find('\"');
		while (pos >= 0 && (ept == -1 || pos < ept)) {
			string label_name(&line[loc], &line[pos]);
			Label[label_name] = MEM_LOW;
			for (++pos; pos < len && (line[pos] == ' ' || line[pos] == '\t' || line[pos] == '\n'); ++pos);
			line.erase(0, pos), len -= pos, loc = 0;
			pos = line.find(':');
			ept = line.find('\"');
		}
		if (!len) continue;
		manageString(line, loc, len);
	}
}

void manageString(string & line, int & loc, int & len) {
	if (line[loc] == '.') {
		int nxt = loc, k, cmp, multime = 1;
		for (; line[nxt] != ' ' && line[nxt] != '\t' && line[nxt] != '\n'; ++nxt);
		string key(&line[loc + 1], &line[nxt]), str;

		// cout << "Debug " << key << '\n';

		// eg: .data  loc: ^data  nxt: .data^
		switch (key[3])
		{
		case('i'): //ascii/asciiz
			for (; line[nxt] == ' ' || line[nxt] == '\t'; ++nxt); loc = nxt;
			for (++nxt; line[nxt] != '\"'; ++nxt);
			str = string(&line[loc + 1], &line[nxt]);
			// cout << "Debug " << str << '\n';
			saveString(str, MEM_LOW);
			//if (key.length() == 5) --MEM_LOW;
			if (key.length() == 6) MEMORY[MEM_LOW++] = '\0';
			break;
		case('e'): //byte
			for (saveInt(toInt(line, loc, len), MEM_LOW, 1); line[loc] == ','; saveInt(toInt(line, ++loc, len), MEM_LOW, 1));
			break;
		case('f'): //half
			for (saveInt(toInt(line, loc, len), MEM_LOW, 2); line[loc] == ','; saveInt(toInt(line, ++loc, len), MEM_LOW, 2));
			break;
		case('d'): //word 
			for (saveInt(toInt(line, loc, len), MEM_LOW, 4); line[loc] == ','; saveInt(toInt(line, ++loc, len), MEM_LOW, 4));
			break;
		case('c'): //space
			MEM_LOW += toInt(line, loc, len);
			break;
		case('g'): //align
			k = toInt(line, loc, len);
			cmp = 2 << (k - 1);
			for (; MEM_LOW > cmp; cmp += (2 << (k - 1)), ++multime);
			MEM_LOW = cmp;
			// cout << "Debug ML = " << MEM_LOW << '\n';
			break;
		default:
			return;
		}
	}
	else {
		int nxt = loc;
		for (; line[nxt] != ' ' && line[nxt] != '\t' && line[nxt] != '\n'; ++nxt);
		string operatorname(&line[loc], &line[nxt]); loc = nxt;
		// cout << "Debug " << operatorname << '\n';
		int _index = OperatorIndex[operatorname];
		//三操作数 Rdest Rsrc1 Rsrc2/Imm
			if (_index <= SNE) saveStruct(line, loc, len, 0, 2, _index, 0);
		//二操作数 Rdest Rsrc1/Imm
		else if (_index <= MOVE) 
			saveStruct(line, loc, len, 0, 1, _index, 0);
		//一操作数 Rdest
		else if (_index <= MFLO) saveStruct(line, loc, len, 0, 0, _index, 0);
		//一操作数 Rsrc1
		else if (_index <= JALR) saveStruct(line, loc, len, 1, 1, _index, 0);
		//地址操作
		else if (_index <= SYSCALL) saveStruct(line, loc, len, 0, -1, _index, 0);
		else if (_index <= DIVU) saveStruct(line, loc, len, 0, 2, _index, 0);
		else if (_index <= JAL) saveStruct(line, loc, len, 0, -1, _index, 1);
		else if (_index <= BLT) saveStruct(line, loc, len, 1, 2, _index, 1);
		else if (_index <= BLTZ) saveStruct(line, loc, len, 1, 1, _index, 1);
		else if (_index <= LW) saveStruct(line, loc, len, 0, 0, _index, 1);
		else saveStruct(line, loc, len, 1, 1, _index, 1);
	}
}

//################################################################################################

int MIPS_SIMULATION() {
	int now_loc = Label["main"];
	OperatorCode _OperatorCode;
	int cnt = 0;
	for (;; now_loc += NXT) {
		_OperatorCode.load(now_loc);

		//Debug(_OperatorCode, cnt, REGISTER);

		switch (_OperatorCode.operatorIndex)
		{
		case(ADD):
		case(ADDU):
		case(ADDIU):
			if (_OperatorCode.Register[2] == EMPTY) REGISTER[_OperatorCode.Register[0]] = REGISTER[_OperatorCode.Register[1]] + _OperatorCode.Imm;
			else REGISTER[_OperatorCode.Register[0]] = REGISTER[_OperatorCode.Register[1]] + REGISTER[_OperatorCode.Register[2]];
			break;
		case(SUB):
		case(SUBU):
			if (_OperatorCode.Register[2] == EMPTY) REGISTER[_OperatorCode.Register[0]] = REGISTER[_OperatorCode.Register[1]] - _OperatorCode.Imm;
			else REGISTER[_OperatorCode.Register[0]] = REGISTER[_OperatorCode.Register[1]] - REGISTER[_OperatorCode.Register[2]];
			break;
		case(XOR):
		case(XORU):
			if (_OperatorCode.Register[2] == EMPTY) REGISTER[_OperatorCode.Register[0]] = REGISTER[_OperatorCode.Register[1]] ^ _OperatorCode.Imm;
			else REGISTER[_OperatorCode.Register[0]] = REGISTER[_OperatorCode.Register[1]] ^ REGISTER[_OperatorCode.Register[2]];
			break;
		case(REM):
			if (_OperatorCode.Register[2] == EMPTY) REGISTER[_OperatorCode.Register[0]] = REGISTER[_OperatorCode.Register[1]] % _OperatorCode.Imm;
			else REGISTER[_OperatorCode.Register[0]] = REGISTER[_OperatorCode.Register[1]] % REGISTER[_OperatorCode.Register[2]];
			break;
		case(REMU): {
			unsigned int src1 = REGISTER[_OperatorCode.Register[1]], src2;
			if (_OperatorCode.Register[2] == EMPTY) src2 = _OperatorCode.Imm;
			else src2 = REGISTER[_OperatorCode.Register[2]];
			REGISTER[_OperatorCode.Register[0]] = src1 % src2;
			break;
		}
		case(SEQ): {
			int tmp;
			if (_OperatorCode.Register[2] == EMPTY) tmp = _OperatorCode.Imm;
			else tmp = REGISTER[_OperatorCode.Register[2]];
			if (REGISTER[_OperatorCode.Register[1]] == tmp) REGISTER[_OperatorCode.Register[0]] = 1;
			else  REGISTER[_OperatorCode.Register[0]] = 0;
			break;
		}
		case(SGE): {
			int tmp;
			if (_OperatorCode.Register[2] == EMPTY) tmp = _OperatorCode.Imm;
			else tmp = REGISTER[_OperatorCode.Register[2]];
			if (REGISTER[_OperatorCode.Register[1]] >= tmp) REGISTER[_OperatorCode.Register[0]] = 1;
			else  REGISTER[_OperatorCode.Register[0]] = 0;
			break;
		}
		case(SGT): {
			int tmp;
			if (_OperatorCode.Register[2] == EMPTY) tmp = _OperatorCode.Imm;
			else tmp = REGISTER[_OperatorCode.Register[2]];
			if (REGISTER[_OperatorCode.Register[1]] > tmp) REGISTER[_OperatorCode.Register[0]] = 1;
			else  REGISTER[_OperatorCode.Register[0]] = 0;
			break;
		}
		case(SLE): {
			int tmp;
			if (_OperatorCode.Register[2] == EMPTY) tmp = _OperatorCode.Imm;
			else tmp = REGISTER[_OperatorCode.Register[2]];
			if (REGISTER[_OperatorCode.Register[1]] <= tmp) REGISTER[_OperatorCode.Register[0]] = 1;
			else  REGISTER[_OperatorCode.Register[0]] = 0;
			break;
		}
		case(SLT): {
			int tmp;
			if (_OperatorCode.Register[2] == EMPTY) tmp = _OperatorCode.Imm;
			else tmp = REGISTER[_OperatorCode.Register[2]];
			if (REGISTER[_OperatorCode.Register[1]] < tmp) REGISTER[_OperatorCode.Register[0]] = 1;
			else  REGISTER[_OperatorCode.Register[0]] = 0;
			break;
		}
		case(SNE): {
			int tmp;
			if (_OperatorCode.Register[2] == EMPTY) tmp = _OperatorCode.Imm;
			else tmp = REGISTER[_OperatorCode.Register[2]];
			if (REGISTER[_OperatorCode.Register[1]] != tmp) REGISTER[_OperatorCode.Register[0]] = 1;
			else  REGISTER[_OperatorCode.Register[0]] = 0;
			break;
		}

		case(NEG):
		case(NEGU):
			REGISTER[_OperatorCode.Register[0]] = -REGISTER[_OperatorCode.Register[1]];
			break;
		case(LI):
			REGISTER[_OperatorCode.Register[0]] = _OperatorCode.Imm;
			break;
		case(MOVE):
			REGISTER[_OperatorCode.Register[0]] = REGISTER[_OperatorCode.Register[1]];
			break;

		case(MFHI):
			REGISTER[_OperatorCode.Register[0]] = REGISTER[HI];
			break;
		case(MFLO):
			REGISTER[_OperatorCode.Register[0]] = REGISTER[LO];
			break;

		case(JR):
			now_loc = REGISTER[_OperatorCode.Register[1]] - NXT;
			break;
		case(JALR):
			REGISTER[31] = now_loc + NXT;
			now_loc = REGISTER[_OperatorCode.Register[1]] - NXT;
			break;

		case(NOP): break;
		case(SYSCALL): {
			switch (REGISTER[V0])
			{
			case(1):
				cout << REGISTER[A0];
				break;
			case(4): {
				int loc = REGISTER[A0];
				while (MEMORY[loc] != '\0') cout << MEMORY[loc++];
				break;
			}
			case(5): 
				cin >> REGISTER[V0];
				break;
			case(8): {
				int loc = REGISTER[A0];
				string str;
				cin >> str;
				if (str.length() > REGISTER[A1] - 1) str = string(&str[0], &str[REGISTER[A1] - 1]);
				saveString(str, loc);
				break;
			}
			case(9):
				REGISTER[V0] = MEM_LOW;
				MEM_LOW += REGISTER[A0];
				break;
			case(10): 
				return 0;
			case(17):
				return REGISTER[A0];
			default:
				break;
			}
			break;
		}

		case(MUL): {
			if (_OperatorCode.Delta == 2) {
				long long tmp = REGISTER[_OperatorCode.Register[0]];
				if (_OperatorCode.Register[1] == EMPTY) tmp *= _OperatorCode.Imm;
				else tmp *= REGISTER[_OperatorCode.Register[1]];
				REGISTER[LO] = int(tmp);
				tmp >>= 32;
				REGISTER[HI] = tmp;
			}
			else {
				long long tmp = REGISTER[_OperatorCode.Register[1]];
				if (_OperatorCode.Register[2] == EMPTY) tmp *= _OperatorCode.Imm;
				else tmp *= REGISTER[_OperatorCode.Register[2]];
				REGISTER[_OperatorCode.Register[0]] = tmp;
			}
			break;
		}
		case(MULU): {
			if (_OperatorCode.Delta == 2) {
				unsigned int src1 = REGISTER[_OperatorCode.Register[0]], src2;
				unsigned long long tmp;
				if (_OperatorCode.Register[1] == EMPTY) src2 = _OperatorCode.Imm;
				else src2 = REGISTER[_OperatorCode.Register[1]];
				tmp = src1 * src2;
				REGISTER[LO] = int(tmp);
				tmp >>= 32;
				REGISTER[HI] = tmp;
			}
			else {
				unsigned int src1 = REGISTER[_OperatorCode.Register[1]], src2;
				if (_OperatorCode.Register[2] == EMPTY) src2 = _OperatorCode.Imm;
				else src2 = REGISTER[_OperatorCode.Register[2]];
				REGISTER[_OperatorCode.Register[0]] = src1 * src2;
			}
			break;
		}
		case(DIV): {
			if (_OperatorCode.Delta == 2) {
				int tmp;
				if (_OperatorCode.Register[1] == EMPTY) tmp = _OperatorCode.Imm;
				else tmp = REGISTER[_OperatorCode.Register[1]];
				REGISTER[LO] = REGISTER[_OperatorCode.Register[0]] / tmp;
				REGISTER[HI] = REGISTER[_OperatorCode.Register[0]] % tmp;
			}
			else {
				int tmp;
				if (_OperatorCode.Register[2] == EMPTY) tmp = _OperatorCode.Imm;
				else tmp = REGISTER[_OperatorCode.Register[2]];
				REGISTER[_OperatorCode.Register[0]] = REGISTER[_OperatorCode.Register[1]] / tmp;
			}
			break;
		}
		case(DIVU): {
			if (_OperatorCode.Delta == 2) {
				unsigned int src1 = REGISTER[_OperatorCode.Register[0]], src2;
				if (_OperatorCode.Register[1] == EMPTY) src2 = _OperatorCode.Imm;
				else src2 = REGISTER[_OperatorCode.Register[1]];
				REGISTER[LO] = src1 / src2;
				REGISTER[HI] = src1 % src2;
			}
			else {
				unsigned int src1 = REGISTER[_OperatorCode.Register[1]], src2;
				if (_OperatorCode.Register[2] == EMPTY) src2 = _OperatorCode.Imm;
				else src2 = REGISTER[_OperatorCode.Register[2]];
				REGISTER[_OperatorCode.Register[0]] = src1 / src2;
			}
			break;
		}

		case(B): 
		case(J): 
			now_loc = _OperatorCode.Label - NXT;
			break;
		case(JAL): 
			REGISTER[31] = now_loc + NXT;
			now_loc = _OperatorCode.Label - NXT;
			break;

		case(BEQ): {
			int tmp;
			if (_OperatorCode.Register[2] == EMPTY) tmp = _OperatorCode.Imm;
			else tmp = REGISTER[_OperatorCode.Register[2]];
			if (REGISTER[_OperatorCode.Register[1]] == tmp) now_loc = _OperatorCode.Label - NXT;
			break;
		}
		case(BNE): {
			int tmp;
			if (_OperatorCode.Register[2] == EMPTY) tmp = _OperatorCode.Imm;
			else tmp = REGISTER[_OperatorCode.Register[2]];
			if (REGISTER[_OperatorCode.Register[1]] != tmp) now_loc = _OperatorCode.Label - NXT;
			break;
		}
		case(BGE): {
			int tmp;
			if (_OperatorCode.Register[2] == EMPTY) tmp = _OperatorCode.Imm;
			else tmp = REGISTER[_OperatorCode.Register[2]];
			if (REGISTER[_OperatorCode.Register[1]] >= tmp) now_loc = _OperatorCode.Label - NXT;
			break;
		}
		case(BLE): {
			int tmp;
			if (_OperatorCode.Register[2] == EMPTY) tmp = _OperatorCode.Imm;
			else tmp = REGISTER[_OperatorCode.Register[2]];
			if (REGISTER[_OperatorCode.Register[1]] <= tmp) now_loc = _OperatorCode.Label - NXT;
			break;
		}
		case(BGT): {
			int tmp;
			if (_OperatorCode.Register[2] == EMPTY) tmp = _OperatorCode.Imm;
			else tmp = REGISTER[_OperatorCode.Register[2]];
			if (REGISTER[_OperatorCode.Register[1]] > tmp) now_loc = _OperatorCode.Label - NXT;
			break;
		}
		case(BLT): {
			int tmp;
			if (_OperatorCode.Register[2] == EMPTY) tmp = _OperatorCode.Imm;
			else tmp = REGISTER[_OperatorCode.Register[2]];
			if (REGISTER[_OperatorCode.Register[1]] < tmp) now_loc = _OperatorCode.Label - NXT;
			break;
		}

		case(BEQZ): 
			if (REGISTER[_OperatorCode.Register[1]] == 0) now_loc = _OperatorCode.Label - NXT;
			break;
		case(BNEZ): 
			if (REGISTER[_OperatorCode.Register[1]] != 0) now_loc = _OperatorCode.Label - NXT;
			break;
		case(BLEZ): 
			if (REGISTER[_OperatorCode.Register[1]] <= 0) now_loc = _OperatorCode.Label - NXT;
			break;
		case(BGEZ): 
			if (REGISTER[_OperatorCode.Register[1]] >= 0) now_loc = _OperatorCode.Label - NXT;
			break;
		case(BGTZ): 
			if (REGISTER[_OperatorCode.Register[1]] > 0) now_loc = _OperatorCode.Label - NXT;
			break;
		case(BLTZ): 
			if (REGISTER[_OperatorCode.Register[1]] < 0) now_loc = _OperatorCode.Label - NXT;
			break;

		case(LA): 
			if (_OperatorCode.Register[1] == EMPTY) REGISTER[_OperatorCode.Register[0]] = _OperatorCode.Label;
			else REGISTER[_OperatorCode.Register[0]] = REGISTER[_OperatorCode.Register[1]] + _OperatorCode.Delta;
			break;
		case(LB): 
			if (_OperatorCode.Register[1] == EMPTY) REGISTER[_OperatorCode.Register[0]] = *((char*)(MEMORY + _OperatorCode.Label));
			else REGISTER[_OperatorCode.Register[0]] = *((char*)(MEMORY + (REGISTER[_OperatorCode.Register[1]] + _OperatorCode.Delta)));
			break;
		case(LH): 
			if (_OperatorCode.Register[1] == EMPTY) REGISTER[_OperatorCode.Register[0]] = *((short*)(MEMORY + _OperatorCode.Label));
			else REGISTER[_OperatorCode.Register[0]] = *((short*)(MEMORY + (REGISTER[_OperatorCode.Register[1]] + _OperatorCode.Delta)));
			break;
		case(LW): 
			if (_OperatorCode.Register[1] == EMPTY) REGISTER[_OperatorCode.Register[0]] = *((int*)(MEMORY + _OperatorCode.Label));
			else REGISTER[_OperatorCode.Register[0]] = *((int*)(MEMORY + (REGISTER[_OperatorCode.Register[1]] + _OperatorCode.Delta)));
			break;

		case(SB): {
			int loc;
			if (_OperatorCode.Register[0] == EMPTY) loc = _OperatorCode.Label;
			else loc = REGISTER[_OperatorCode.Register[0]] + _OperatorCode.Delta;
			saveInt(REGISTER[_OperatorCode.Register[1]], loc, 1);
			break;
		}
		case(SH): {
			int loc;
			if (_OperatorCode.Register[0] == EMPTY) loc = _OperatorCode.Label;
			else loc = REGISTER[_OperatorCode.Register[0]] + _OperatorCode.Delta;
			saveInt(REGISTER[_OperatorCode.Register[1]], loc, 2);
			break;
		}
		case(SW): {
			int loc;
			if (_OperatorCode.Register[0] == EMPTY) loc = _OperatorCode.Label;
			else loc = REGISTER[_OperatorCode.Register[0]] + _OperatorCode.Delta;
			saveInt(REGISTER[_OperatorCode.Register[1]], loc, 4);
			break;
		}
		default:
			break;
		}
	}
}