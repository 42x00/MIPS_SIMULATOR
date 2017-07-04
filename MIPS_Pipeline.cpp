#include "MIPS_Pipeline.h"

int main(int argc, char *argv[])
{
	fin = ifstream(argv[1]);
	MIPS_PRETREATMENT();
	return MIPS_SIMULATOR();
}

//########################################  转换  ################################################

int toInt(string & line, int & loc, int len) {
	int ret = 0, f = 1;
	for (; !(line[loc] == '-' || (line[loc] >= '0' && line[loc] <= '9')); ++loc);
	if (line[loc] == '-') f = -1, ++loc;
	for (; loc < len && line[loc] >= '0' && line[loc] <= '9'; ret = ret * 10 + line[loc] - '0', ++loc);
	return ret * f;
}

int toRInt(string & line, int &loc, int len) {
	for (; line[loc] != '$'; ++loc);
	++loc;
	if (line[loc] >= '0' && line[loc] <= '9') {
		int ret = 0;
		for (; line[loc] >= '0' && line[loc] <= '9'; ret = ret * 10 + line[loc] - '0', ++loc);
		return ret;
	}
	else {
		int nxt = loc;
		for (; line[nxt] != ' ' && line[nxt] != '\t' && line[nxt] != '\n' && line[nxt] != ',' && line[nxt] != ')'; ++nxt);
		string RegisterName(&line[loc], &line[nxt]); loc = nxt;
		return RegisterIndex[RegisterName];
	}
}

void saveInt(int x, int & loc, int ord) {
	if (ord == 1) new(MEMORY + loc) char(x);
	else if (ord == 2) new(MEMORY + loc) short(x);
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
	MEMORY[loc++] = '\0';
}

void saveStruct(string & line, int & loc, int len, int l, int r, int index, bool label) {
	OperatorCode tmp(index); int i = l;
	for (; i <= r; ++i) {
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
		}
	}
	new(MEMORY + MEM_LOW) OperatorCode(tmp);
	MEM_LOW += NXT;
}

//#######################################  预处理  ###############################################

void MIPS_PRETREATMENT() {
	prepare();
	code_in();
	code_scanf();
	MEM_LOW = 0;
	code_scanf();
	REGISTER[SP] = M - 1;
}

void code_in() {
	string line;
	while (getline(fin, line)) line.push_back('\n'), Code.push_back(line);
}

void prepare() {
	for (int i = 0; i < forOperator.size(); ++i) OperatorIndex[forOperator[i]] = i;
	for (int i = 0; i < forRegister.size(); ++i) RegisterIndex[forRegister[i]] = i;
	RegisterIndex["s8"] = 30;
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
		// eg: .data  loc: ^data  nxt: .data^
		switch (key[3])
		{
		case('i'): //ascii/asciiz
			for (; line[nxt] == ' ' || line[nxt] == '\t'; ++nxt); loc = nxt;
			for (++nxt; line[nxt] != '\"'; ++nxt);
			str = string(&line[loc + 1], &line[nxt]);
			saveString(str, MEM_LOW);
			if (key.length() == 5) --MEM_LOW;
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
			break;
		default:
			return;
		}
	}
	else {
		int nxt = loc;
		for (; line[nxt] != ' ' && line[nxt] != '\t' && line[nxt] != '\n'; ++nxt);
		string operatorname(&line[loc], &line[nxt]); loc = nxt;
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
		else if (_index <= SW) saveStruct(line, loc, len, 1, 1, _index, 1);
	}
}

//######################################  Pipeline  ################################################

int MIPS_SIMULATOR() {
	PC = Label["main"];
	while (SIMULATOR) {
		_WriteBack.go();
		_MemoryAccess.go();
		_Execution.go();
		_DataPreparation.go();
		_InstructionFetch.go();
	}
	return SIMULATOR_RET;
}

void DataPreparation::go() {
	if (_OperatorCode.operatorIndex == -1) return;
	block = false, _InstructionFetch.block = false;
	for (int i = 0; i < 3; ++i) tovis[i] = false;
	switch (_OperatorCode.operatorIndex)
	{
	case(ADD):
	case(ADDU):
	case(ADDIU):
	case(SUB):
	case(SUBU):
	case(XOR):
	case(XORU):
	case(REM):
	case(REMU):
	case(SEQ):
	case(SGE):
	case(SGT):
	case(SLE):
	case(SLT):
	case(SNE):
	case(BEQ):
	case(BGE):
	case(BGT):
	case(BLE):
	case(BLT):
	case(BNE):
		tovis[1] = tovis[2] = true;
		break;
	case(NEG):
	case(NEGU):
	case(MOVE):
	case(JR):
	case(JALR):
	case(BEQZ):
	case(BGEZ):
	case(BGTZ):
	case(BLEZ):
	case(BLTZ):
	case(BNEZ):
	case(LA):
	case(LB):
	case(LH):
	case(LW):
		tovis[1] = true;
		break;
	case(MFHI):
		if (vis[HI]) {
			Stop_DataPreparation();
			return;
		}
		else _OperatorCode.Imm = REGISTER[HI];
		break;
	case(MFLO):
		if (vis[LO]) {
			Stop_DataPreparation();
			return;
		}
		else _OperatorCode.Imm = REGISTER[LO];
		break;
	case(MUL):
	case(MULU):
	case(DIV):
	case(DIVU):
		if (_OperatorCode.Delta==2) tovis[0] = tovis[1] = true;
		else tovis[1] = tovis[2] = true;
		break;
	case(SB):
	case(SH):
	case(SW):
		tovis[0] = tovis[1] = true;
		break;
	case(SYSCALL): {
		if (vis[V0]) {
			Stop_DataPreparation();
			return;
		}
		else {
			_OperatorCode.Delta = REGISTER[V0];
			switch (_OperatorCode.Delta)
			{
			case(1):
				if (vis[A0]) {
					Stop_DataPreparation();
					return;
				}
				else _OperatorCode.Imm = REGISTER[A0];
				break;
			case(4):
				if (vis[A0]) {
					Stop_DataPreparation();
					return;
				}
				else _OperatorCode.Label = REGISTER[A0];
				break;
			case(8):
				if (vis[A0] || vis[A1]) {
					Stop_DataPreparation();
					return;
				}
				else _OperatorCode.Imm = REGISTER[A1], _OperatorCode.Label = REGISTER[A0];
				break;
			case(9):
				if (vis[A0]) {
					Stop_DataPreparation();
					return;
				}
				else _OperatorCode.Imm = REGISTER[A0];
				break;
			case(10):
				break;
			case(17):
				if (vis[A0]) {
					Stop_DataPreparation();
					return;
				}
				else _OperatorCode.Imm = REGISTER[A0];
				break;
			default:
				break;
			}
		}
	}
	default:
		break;
	}
	
	for (int i = 0; i < 3; ++i) {
		if (tovis[i] && _OperatorCode.Register[i] != EMPTY && vis[_OperatorCode.Register[i]]) {
			Stop_DataPreparation();
			break;
		}
	}

	if (!block) {
		for (int i = 0; i < 3; ++i) {
			if (tovis[i]) {
				if (_OperatorCode.Register[i] == EMPTY) Register[i] = _OperatorCode.Imm;
				else Register[i] = REGISTER[_OperatorCode.Register[i]];
			}
		}
		switch (_OperatorCode.operatorIndex)
		{
		case(ADD):
		case(ADDU):
		case(ADDIU):
		case(SUB):
		case(SUBU):
		case(XOR):
		case(XORU):
		case(REM):
		case(REMU):
		case(SEQ):
		case(SGE):
		case(SGT):
		case(SLE):
		case(SLT):
		case(SNE):
		case(NEG):
		case(NEGU):
		case(LI):
		case(MOVE):
		case(MFHI):
		case(MFLO):
		case(LA):
		case(LB):
		case(LH):
		case(LW):
			vis[_OperatorCode.Register[0]] = true;
			break;
		case(JAL):
		case(JALR):
			vis[31] = true;
			break;
		case(MUL):
		case(MULU):
		case(DIV):
		case(DIVU):
			if (_OperatorCode.Delta == 2) vis[LO] = vis[HI] = true;
			else vis[_OperatorCode.Register[0]] = true;
			break;
		case(SYSCALL): {
			switch (_OperatorCode.Delta)
			{
			case(5):
			case(9):
				vis[V0] = true;
				break;
			default:
				break;
			}
		}
		default:
			break;
		}
		_Execution.load(_OperatorCode, Register);
	}
}

void DataPreparation::Stop_DataPreparation() {
	block = true;
	_InstructionFetch.block = true;
}

void Execution::go() {
	if (_OperatorCode.operatorIndex == -1) return;
	switch (_OperatorCode.operatorIndex)
	{
	case(ADD):
	case(ADDU):
	case(ADDIU):
		_OperatorCode.Imm = Register[1] + Register[2];
		break;
	case(SUB):
	case(SUBU):
		_OperatorCode.Imm = Register[1] - Register[2];
		break;
	case(XOR):
	case(XORU):
		_OperatorCode.Imm = Register[1] ^ Register[2];
		break;
	case(REM):
		_OperatorCode.Imm = Register[1] % Register[2];
		break;
	case(REMU): {
		unsigned int src1 = Register[1], src2 = Register[2];
		_OperatorCode.Imm = src1 % src2;
		break;
	}
	case(SEQ): 
		_OperatorCode.Imm = Register[1] == Register[2];
		break;
	case(SGE):
		_OperatorCode.Imm = Register[1] >= Register[2];
		break;
	case(SGT): 
		_OperatorCode.Imm = Register[1] > Register[2];
		break;
	case(SLE): 
		_OperatorCode.Imm = Register[1] <= Register[2];
		break;
	case(SLT): 
		_OperatorCode.Imm = Register[1] < Register[2];
		break;
	case(SNE): 
		_OperatorCode.Imm = Register[1] != Register[2];
		break;
	case(NEG):
	case(NEGU):
		_OperatorCode.Imm = -Register[1];
		break;
	case(MOVE):
		_OperatorCode.Imm = Register[1];
		break;
	case(MFHI):
		_OperatorCode.Imm = REGISTER[HI];
		break;
	case(MFLO):
		_OperatorCode.Imm = REGISTER[LO];
		break;
	case(JR):
		_OperatorCode.Label = Register[1];
		break;
	case(JALR):
		_OperatorCode.Label = Register[1];
		break;
	case(MUL): {
		if (_OperatorCode.Delta == 2) {
			long long tmp = Register[0] * Register[1];
			_OperatorCode.Imm = int(tmp);
			tmp >>= 32;
			_OperatorCode.Label = tmp;
		}
		else {
			long long tmp = Register[1] * Register[2];
			_OperatorCode.Imm = tmp;
		}
		break;
	}
	case(MULU): {
		if (_OperatorCode.Delta == 2) {
			unsigned int src1 = Register[0], src2 = Register[1];
			unsigned long long tmp = src1 * src2;
			_OperatorCode.Imm = int(tmp);
			tmp >>= 32;
			_OperatorCode.Label = tmp;
		}
		else {
			unsigned int src1 = Register[1], src2 = Register[2];
			_OperatorCode.Imm = src1 * src2;
		}
	}
	case(DIV): {
		if (_OperatorCode.Delta == 2) {
			_OperatorCode.Imm = Register[0] / Register[1];
			_OperatorCode.Label = Register[0] % Register[1];
		}
		else _OperatorCode.Imm = Register[1] / Register[2];
		break;
	}
	case(DIVU): {
		if (_OperatorCode.Delta == 2) {
			unsigned int src1 = Register[0], src2 = Register[1];
			_OperatorCode.Imm = src1 / src2;
			_OperatorCode.Label = src1 % src2;
		}
		else {
			unsigned int src1 = Register[1], src2 = Register[2];
			_OperatorCode.Imm = src1 / src2;
		}
		break;
	}
	case(BEQ): 
		_OperatorCode.Imm = Register[1] == Register[2];
		break;
	case(BGE):
		_OperatorCode.Imm = Register[1] >= Register[2];
		break;
	case(BGT):
		_OperatorCode.Imm = Register[1] > Register[2];
		break;
	case(BLE):
		_OperatorCode.Imm = Register[1] <= Register[2];
		break;
	case(BLT):
		_OperatorCode.Imm = Register[1] < Register[2];
		break;
	case(BNE):
		_OperatorCode.Imm = Register[1] != Register[2];
		break;
	case(BEQZ):
		_OperatorCode.Imm = Register[1] == 0;
		break;
	case(BNEZ):
		_OperatorCode.Imm = Register[1] != 0;
		break;
	case(BLEZ):
		_OperatorCode.Imm = Register[1] <= 0;
		break;
	case(BGEZ):
		_OperatorCode.Imm = Register[1] >= 0;
		break;
	case(BGTZ):
		_OperatorCode.Imm = Register[1] > 0;
		break;
	case(BLTZ):
		_OperatorCode.Imm = Register[1] < 0;
		break;
	case(LA):
	case(LB):
	case(LH):
	case(LW):
		if (_OperatorCode.Register[1] != EMPTY) _OperatorCode.Label = Register[1] + _OperatorCode.Delta;
		break;
	case(SB):
	case(SH):
	case(SW):
		if (_OperatorCode.Register[0] != EMPTY) _OperatorCode.Label = Register[0] + _OperatorCode.Delta;
		_OperatorCode.Imm = Register[1];
		break;
	default:
		break;
	}
	_MemoryAccess.load(_OperatorCode);
}

void MemoryAccess::go() {
	if (_OperatorCode.operatorIndex == -1) return;
	switch (_OperatorCode.operatorIndex)
	{
	case(LA):
		_OperatorCode.Imm = _OperatorCode.Label;
		break;
	case(LB):
		_OperatorCode.Imm = *((char*)(MEMORY + _OperatorCode.Label));
		break;
	case(LH):
		_OperatorCode.Imm = *((short*)(MEMORY + _OperatorCode.Label));
		break;
	case(LW):
		_OperatorCode.Imm = *((int*)(MEMORY + _OperatorCode.Label));
		break;
	case(SB): {
		int loc = _OperatorCode.Label;
		saveInt(_OperatorCode.Imm, loc, 1);
		break;
	}
	case(SH): {
		int loc = _OperatorCode.Label;
		saveInt(_OperatorCode.Imm, loc, 2);
		break;
	}
	case(SW): {
		int loc = _OperatorCode.Label;
		saveInt(_OperatorCode.Imm, loc, 4);
		break;
	}
	case(SYSCALL): {
		switch (_OperatorCode.Delta)
		{
		case(1):
			cout << _OperatorCode.Imm;
			break;
		case(4): {
			for (int loc = _OperatorCode.Label; MEMORY[loc] != '\0'; cout << MEMORY[loc], ++loc);
			break;
		}
		case(5):
			cin >> _OperatorCode.Imm;
			break;
		case(8): {
			string str;
			cin >> str;
			if (str.length() > _OperatorCode.Imm - 1) str = string(&str[0], &str[_OperatorCode.Imm - 1]);
			int loc = _OperatorCode.Label;
			saveString(str, loc);
			break;
		}
		case(9):
			_OperatorCode.Label = MEM_LOW;
			MEM_LOW += _OperatorCode.Imm;
			break;
		default:
			break;
		}
	}
	default:
		break;
	}
	_WriteBack.load(_OperatorCode);
}

void WriteBack::go() {
	if (_OperatorCode.operatorIndex == -1) return;
	switch (_OperatorCode.operatorIndex)
	{
	case(ADD):
	case(ADDU):
	case(ADDIU):
	case(SUB):
	case(SUBU):
	case(XOR):
	case(XORU):
	case(REM):
	case(REMU):
	case(SEQ):
	case(SGE):
	case(SGT):
	case(SLE):
	case(SLT):
	case(SNE):
	case(NEG):
	case(NEGU):
	case(LI):
	case(MOVE):
	case(MFHI):
	case(MFLO):
	case(LA):
	case(LB):
	case(LH):
	case(LW):
		REGISTER[_OperatorCode.Register[0]] = _OperatorCode.Imm;
		vis[_OperatorCode.Register[0]] = false;
		break;
	case(JR):
		PC = _OperatorCode.Label;
		_InstructionFetch.jump_block = false;
		break;
	case(JALR):
		REGISTER[31] = PC;
		PC = _OperatorCode.Label;
		_InstructionFetch.jump_block = false;
		vis[31] = false;
		break;
	case(MUL):
	case(MULU):
	case(DIV):
	case(DIVU):
		if (_OperatorCode.Delta == 2) {
			REGISTER[LO] = _OperatorCode.Imm;
			REGISTER[HI] = _OperatorCode.Label;
			vis[LO] = vis[HI] = false;
		}
		else REGISTER[_OperatorCode.Register[0]] = _OperatorCode.Imm, vis[_OperatorCode.Register[0]] = false;
		break;
	case(B):
	case(J):
		PC = _OperatorCode.Label;
		_InstructionFetch.jump_block = false;
		break;
	case(JAL):
		REGISTER[31] = PC;
		PC = _OperatorCode.Label;
		_InstructionFetch.jump_block = false;
		vis[31] = false;
		break;
	case(BEQ):
	case(BGE):
	case(BGT):
	case(BLE):
	case(BLT):
	case(BNE):
	case(BEQZ):
	case(BGEZ):
	case(BGTZ):
	case(BLEZ):
	case(BLTZ):
	case(BNEZ):
		if (_OperatorCode.Imm) PC = _OperatorCode.Label;
		_InstructionFetch.jump_block = false;
		break;
	case(SYSCALL): {
		switch (_OperatorCode.Delta)
		{
		case(5): 
			REGISTER[V0] = _OperatorCode.Imm;
			vis[V0] = false;
			break;
		case(9):
			REGISTER[V0] = _OperatorCode.Label;
			vis[V0] = false;
			break;
		case(10):
			SIMULATOR = false;
			return;
		case(17):
			SIMULATOR = false;
			SIMULATOR_RET = _OperatorCode.Imm;
			return;
		default:
			break;
		}
	}
	default:
		break;
	}
	_OperatorCode.clear();
}

