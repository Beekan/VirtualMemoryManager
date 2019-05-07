// VirtualMemoryManager.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <fstream>
#include <bitset>
#include"math.h"
#include"queue"
#include"string"
using namespace std;

struct PageTablePairs {
	int PageNO;
	int FrameNO;
	bool validbit=0;
};

struct TLBtablePairs {
	string VirtualAddress;
	string PhysicalAddress;
};

string decToBinary(int n, int size)
{
	string out;
	for (int i = size-1; i >= 0; i--) {
		int k = n >> i;
		if (k & 1)
			out=out+ "1";
		else
			out = out + "0";
	}
	return out;
}

int main()
{

	cout << "This is a simulation of a virtual memory manager which incorporates the paging method" << endl;

	//--Getting inputs from the user and declaring tables
	int mode;
	int inputtype;
	ifstream inputFile, MemoryFile;
	ofstream outputFile;
	string FileName,outFileName;
	string MemoryName="BACKING_STORE.bin";
	MemoryFile.open(MemoryName);
	if (!MemoryFile)return 0;
	cout << "Do you want a manual run of the simulation or a default run?: 1.Manual 2.Default: ";
	cin >> mode; cout << endl;
	cout << "Do you want to input Manually input the address or through a file?: 1.Manual 2.file: ";
	cin >> inputtype; cout << endl;
	if (inputtype == 2) {
		cout << "Please,Enter the File Name: ";
		cin>> FileName;
		inputFile.open(FileName);
		while(!inputFile) {
			cout << "Please,Enter a valid File Name: ";
			cin >> FileName;
			inputFile.open(FileName);
		}
	}
	cout << "Please, Enter the output File Name: ";
	cin >> outFileName; cout << endl;
	outputFile.open(outFileName);
	outputFile << "Input Virtual Memory address in decimal" << "   " << "Output Physical Memory address in decimal" << endl;
	
	int PageSize=256;
	int NumberOfPages = 256;
	int MemorySize=65536;
	int NumberOfFrames = 256;
	int TLBenteries=16;
	int PagesbitSize = 8;
	int PagesizebitSize = 8;
	string outputBin;
	int outputDec;
	bool TLBhit;
	int PageFaultRate;
	int TLBhitRate;
	int TLBTurns = 0;
	int PageTurns = 0;
	int FreeFrameNumber;
	if (mode == 1) {
		cout << "please,Enter the Number of pages: ";
		cin >> NumberOfPages; cout << endl;
		cout << "please,Enter the page size: ";
		cin >> PageSize; cout << endl;
		cout << "please,Enter the Physical Memory size: ";
		cin >> MemorySize; cout << endl;
		cout << "please,Enter the TLB table size: ";
		cin >> TLBenteries; cout << endl;
		PagesbitSize = ceil(log2(NumberOfPages))+1;
		PagesizebitSize = ceil(log2(PageSize))+1;
		NumberOfFrames = MemorySize / PageSize;
	}
	int virtualMemorySize = pow(2, log2(NumberOfPages) + log2(PageSize));
	int* FreeFrames = new int[NumberOfFrames - 1];
	PageTablePairs* Pagetable=new PageTablePairs[NumberOfPages-1];
	TLBtablePairs* TLBtable=new TLBtablePairs[TLBenteries-1];
	char* PhysicalMemory = new char[MemorySize];
	int currentFreeFrameindex = 0;
	int currentTLBtableindex = 0;

	//--Initializing tables
	for (int i = 0; i < NumberOfFrames - 1; i++) {
		FreeFrames[i] = -1;
	}

	for (int i = 0; i < TLBenteries-1; i++) {
		TLBtable[i].PhysicalAddress = "-1";
	}

	
	while (true) {
		TLBhit=0;
		//--Reading Memory input from user
		int MemoryInputdec;
		string MemoryInputBin;
		if (inputtype == 1) {
			cout << "Please,Enter the Logical address you want to access or -1 to exit : " << endl;
			cin >> MemoryInputdec; cout << endl;

			if (MemoryInputdec == -1)break;

			while (MemoryInputdec > virtualMemorySize || MemoryInputdec < 0) {
				cout << "Input Virtual address is invalid. Please enter another: " << endl;
				cin >> MemoryInputdec; cout << endl;
			}

			MemoryInputBin = decToBinary(MemoryInputdec, PagesbitSize + PagesizebitSize);
		}
		else {
			if (inputFile.eof())break;

			inputFile >> MemoryInputdec;
			if (MemoryInputdec > virtualMemorySize || MemoryInputdec < 0) {
				cout << "Input File has incorrect addresses" << endl;
				break;
			}
			MemoryInputBin = decToBinary(MemoryInputdec, PagesbitSize + PagesizebitSize);

		}




		//--translating the input
		int InputPageNO = stoi(MemoryInputBin.substr(0, PagesbitSize), nullptr, 2);
		int InputPageOffset = stoi(MemoryInputBin.substr(PagesbitSize+1, MemoryInputBin.length()), nullptr, 2);

		//--Checking validity
		while (InputPageNO > NumberOfPages || InputPageOffset > PageSize) {
			cout << "Input Virtual address is invalid.";
			if (inputtype == 1) {
				cout<<" Please enter another: " << endl;
				cin >> MemoryInputdec; cout << endl;
				MemoryInputBin = decToBinary(MemoryInputdec, PagesbitSize + PagesizebitSize);
				InputPageNO = stoi(MemoryInputBin.substr(0, PagesbitSize), nullptr, 2);
				InputPageOffset = stoi(MemoryInputBin.substr(PagesbitSize+1 , MemoryInputBin.length()), nullptr, 2);
			}
			else break;
		}
		//--Searching TLB table for already existing pairs
		for(int i=0;i<TLBenteries-1;i++) {
			if (!TLBtable[i].VirtualAddress.compare(MemoryInputBin.substr(0, PagesbitSize))) {
				outputBin = TLBtable[i].PhysicalAddress + decToBinary(InputPageOffset,PagesizebitSize);
				outputDec = stoi(outputBin, nullptr, 2);
				outputFile << MemoryInputdec << "                  " << outputDec << endl;
				TLBhit = 1;
				break;
			}
			

		}

		//--Searching Page table
		if (TLBhit == 0) {
			for (int i = 0; i < NumberOfPages; i++) {
				if (i== InputPageNO && Pagetable[i].validbit == 1) {
					outputBin = decToBinary(Pagetable[i].FrameNO, PagesbitSize) + decToBinary(InputPageOffset, PagesbitSize);
					outputDec = stoi(outputBin, nullptr, 2);
					outputFile << MemoryInputdec << "                  " << outputDec << endl;

					//code for updating TLB
					TLBtable[currentTLBtableindex].VirtualAddress = MemoryInputBin.substr(0, PagesbitSize);
					TLBtable[currentTLBtableindex].PhysicalAddress = outputBin.substr(0, PagesbitSize);
					currentTLBtableindex = (currentTLBtableindex + 1) % (TLBenteries-1);
					break;
				}
				else if (i == InputPageNO) {
					//code for page fault
					Pagetable[i].FrameNO = currentFreeFrameindex;
					currentFreeFrameindex = (currentFreeFrameindex + 1) % (NumberOfFrames-1);
					Pagetable[i].validbit = 1;
					i--;
					
					continue;

					
				}
				


			}
		}
		
	}
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
