#include <SlyvQCol.hpp>
#include <SlyvQuickHead.hpp>
#include <SlyvVolumes.hpp>
#include <SlyvDirry.hpp>
#include <SlyvStream.hpp>
#include <SlyvDir.hpp>

MKL_Init

using namespace std;
using namespace Slyvina;
using namespace Units;

#pragma region WorkClass
class FileRec {
private:
	static map<string, shared_ptr<FileRec>> Register;
	uint64 _CheckSum{ 0 };
	streampos _Size{ 0 };
	bool _scannedSize{ false };
	bool _scanned{ false };
	string _FileName{ "" }, _TrueFileName{ "" };
	VecString _Dupes{ NewVecString() };
public:
	static shared_ptr < FileRec > Obtain(std::string);
	static bool Compare(shared_ptr < FileRec > A, shared_ptr < FileRec > B);
	static bool Compare(std::string A, std::string B) { return Compare(Obtain(A), Obtain(B)); }
	static void Results();
	VecString Dupes() { return _Dupes; }
	bool Compare(std::string W) { return Compare(_FileName, W); }
	void Scan(bool SizeOnly = false);
	void CheckSum(uint64 c) { _CheckSum = c; _scanned = true; }
	uint64 CheckSum() {
		if (!_scanned) Scan();
		return _CheckSum;
	}
	streampos Size() {
		if (!_scannedSize) Scan(true);
		return _Size;
	}
	bool Scanned() { return _scanned; }
	FileRec(string F) { _FileName = F; _TrueFileName = AVolPath(Dirry(F)); }
	string TrueFileName() { return _TrueFileName; }
	string FileName() { return _FileName; }
};

map<string, shared_ptr<FileRec>> FileRec::Register{};

void FileRec::Scan(bool SizeOnly) {
	_Size = FileSize(_TrueFileName);
	if (SizeOnly) return;
	QCol->Doing("Scanning", _FileName);
	_CheckSum = 0;
	auto BT = ReadFile(_TrueFileName);
	while (!BT->EndOfFile()) _CheckSum += BT->ReadByte();
	BT->Close();
}

shared_ptr<FileRec> FileRec::Obtain(std::string F) {
	if (!Register.count(F)) Register[F] = shared_ptr<FileRec>(new FileRec(F));
	return Register[F];
}

bool FileRec::Compare(shared_ptr < FileRec > A, shared_ptr < FileRec > B) {
	if (A->Size() != B->Size()) return false;
	if (A->Size() == 0 && B->Size() == 0) return true;
	if (A->Scanned() && B->Scanned() && A->CheckSum() != B->CheckSum()) return true;
	QCol->Yellow("Comparing:\n\t");
	QCol->Cyan(A->FileName());
	QCol->LMagenta(" with: \n\t");
	QCol->Cyan(B->FileName());
	QCol->Dark("\n");
	auto
		BTA{ ReadFile(A->TrueFileName()) },
		BTB{ ReadFile(B->TrueFileName()) };
	uint64 P{ 0 }, CSA{ 0 }, CSB{ 0 };
	auto ret = true;	
	while (!BTA->EndOfFile()) {
		if (P++ % 5000 == 0) { QCol->Green(TrSPrintF("%5.1f%%\r", ((double)P / (double)BTA->Size()) * 100)); }
		auto XA{ BTA->ReadByte() }, XB{ BTB->ReadByte() };
		if (XA != XB) { ret = false; break; }
		CSA += XA;
		CSB += XB;
	}
	BTA->Close();
	BTB->Close();
	QCol->Dark("                                    \r");
	if (ret) {
		A->CheckSum(CSA);
		B->CheckSum(CSB);
		QCol->LGreen("\r\tMatch!\n");
	} else QCol->Red("\r\tNo match!\n");
	return ret;
}

void FileRec::Results() {
	size_t Total{ 0 };
	for (auto FRI : Register) {
		auto FR{ FRI.second };
		switch(FR->Dupes()->size()) {
		case 0: break;
		case 1: QCol->LGreen(FR->FileName()); QCol->Yellow(" has "); QCol->Cyan("1 "); QCol->Yellow("dupe\n"); break;
		default:QCol->LGreen(FR->FileName()); QCol->Yellow(" has "); QCol->Cyan(to_string(FR->Dupes()->size())); QCol->Yellow(" dupes\n"); break;
		}
		for (size_t i = 0; i < FR->Dupes()->size();++i) {
			QCol->Cyan(TrSPrintF("%9d: ", i));
			QCol->Yellow((*FR->Dupes())[i]);
			QCol->Grey("\n");
			Total++;
		}
	}
	QCol->Doing("Total dupes", to_string(Total),"\n\n");
}

#pragma endregion
VecString GetDir(String D) {
	QCol->Doing("Reading dir", D,"");
	QCol->LGreen(" ... ");
	auto ret{ GetTree(AVolPath(Dirry(D))) };
	switch (ret->size()) {
	case 0: QCol->Red("NOTHING AT ALL!\n"); break;
	case 1: QCol->LMagenta("1 file\n"); break;
	default:QCol->LMagenta(TrSPrintF("%d files\n",ret->size())); break;
	}
	return ret;
}



#pragma region Main
int main(int countargs, char** args) {
	MKL_Version("Gates", "00.00.00"); // Will soon be deprecated!
	MKL_Lic("Gates", "GPL3"); // Will soon be deprecated!
	QuickHeader("Gates", 2024);
	if (countargs < 3) {
		QCol->Red("Usage: ");
		QCol->Grey(StripAll(args[0]));
		QCol->Cyan(" <Base dir> <Dir 1> ");
		QCol->Magenta("[<Dir 2> [<Dir 3>....]]\n\n");
		QCol->Reset();
		return 0;
	}
	auto MainD{ GetDir(args[1])};
	for (int i = 2; i < countargs; ++i) {
		auto WorkD{ GetDir(args[i]) };
		QCol->Doing("Comparing Dir", args[i]);
		for (size_t WDI = 0; WDI < WorkD->size(); ++WDI) {
			if (WDI % 100 == 0) {
				auto RI{ WDI / 100 };
				auto RID{ (double)WDI / (double)WorkD->size() };
				QCol->Magenta(TrSPrintF("%5.1f%%\r", (RID) * 100));
			}
			for (auto F : *MainD) {
				if (FileRec::Compare(string(args[1]) + "/" + F, string(args[i]) + "/" + (*WorkD)[WDI])) FileRec::Obtain(string(args[1]) + "/" + F)->Dupes()->push_back(string(args[i]) + "/" + (*WorkD)[WDI]);
			}
		}
	}
	FileRec::Results();
	QCol->Reset();
	return 0;
}
#pragma endregion