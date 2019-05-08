// VirtualMemoryManager.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <cstdio>
#include <fstream>
#include <bitset>
#include"math.h"
#include"queue"
#include"string"
using namespace std;

struct PageTablePairs {
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

	//--Declaring variables

	int mode,InputType;                   // mode:used to determine whether the settings are going to be set manually or not  InputType:either from file or written through command 
	
	int PageSize = 256;                   // Memory Properties
	int NumberOfPages = 256;
	int MemorySize = 65536;
	int NumberOfFrames = 256;
	int TLBenteries = 16;
	int PagesbitSize = 8;
	int PagesizebitSize = 8;
	
	string outputBin;                     // Memory address output in binary
	int outputDec;                        // Memory address output in decimal
	int MemoryInputdec;                   // Memory address input in decimal
	string MemoryInputBin;                // Memory address input in binary
	bool TLBhit;                          // used to determine whether the output was gained from TLB table or not 
	int TLBhitNumber = 0;                 // TLB hit amount
	int PageFaultNumber = 0;              // Amount of page Fault occurences   
	int InputAddressAmount=0;



	//--Opening Files using the names collected from the user using Fstream 

	ifstream inputFile;
	ofstream outputFile;
	string FileName, outFileName;

	FILE* MemoryFile = fopen("BACKING_STORE.bin", "rb");
	if (!MemoryFile) return 0;

	cout << "Do you want a manual run of the simulation or a default run?: 1.Manual 2.Default: ";
	cin >> mode; cout << endl;
	cout << "Do you want to input Manually input the address or through a file?: 1.Manual 2.file: ";
	cin >> InputType; cout << endl;
	
	if (InputType == 2) {
		cout << "Please,Enter the File Name: ";
		cin>> FileName;
		inputFile.open(FileName);
		while(!inputFile) {                                  //while loop to check validity 
			cout << "Please,Enter a valid File Name: ";
			cin >> FileName;
			inputFile.open(FileName);
		}
	}
	cout << "Please, Enter the output File Name: ";
	cin >> outFileName; cout << endl;
	outputFile.open(outFileName);
	
	
	
	//--Collecting info from user about the Memory settings if the manual mode was chosen

	if (mode == 1) {
		cout << "please,Enter the Number of pages: ";
		cin >> NumberOfPages; cout << endl;
		cout << "please,Enter the page size: ";
		cin >> PageSize; cout << endl;
		cout << "please,Enter the Number Of frames: ";
		cin >> NumberOfFrames; cout << endl;
		cout << "please,Enter the TLB table size: ";
		cin >> TLBenteries; cout << endl;
		PagesbitSize = ceil(log2(NumberOfPages));                   //number of bits used to describe the Page number
		PagesizebitSize = ceil(log2(PageSize));                     //number of bits used to describe the page size(offset)
		MemorySize=PageSize*NumberOfFrames;
	}



	//--Declaring tables and variables that assist table organization

	int virtualMemorySize = pow(2, log2(NumberOfPages) + log2(PageSize));
	int* FreeFrames = new int[NumberOfFrames - 1];
	PageTablePairs* Pagetable=new PageTablePairs[NumberOfPages-1];      
	TLBtablePairs* TLBtable=new TLBtablePairs[TLBenteries-1];
	char* PhysicalMemory = new char[MemorySize];
	char* buffer = new char[PageSize];
	int currentFreeFrameindex = 0;                                     // A counter to know which Frame Number is going to be replaced next using FIFO
	int currentTLBtableindex = 0;                                      // A counter to know which table input is going to be replaced next using FIFO

	


	//--While statement that hold all the simulation together

	while (true) {
		TLBhit=0;                


		//--Reading Memory input from user
		
		if (InputType == 1) {
			cout << "Please,Enter the Logical address you want to access or -1 to exit : " << endl;
			cin >> MemoryInputdec; cout << endl;
			InputAddressAmount++;
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
			InputAddressAmount++;
		}




		//--translating the input
		int InputPageNO = stoi(MemoryInputBin.substr(0, PagesbitSize), nullptr, 2);
		int InputPageOffset = stoi(MemoryInputBin.substr(PagesbitSize, MemoryInputBin.length()), nullptr, 2);


		//--Checking validity of the input address
		while (InputPageNO > NumberOfPages || InputPageOffset > PageSize) {
			if (InputType == 1) {
				cout<<"Input Virtual address is invalid. Please enter another: " << endl;
				cin >> MemoryInputdec; cout << endl;
				MemoryInputBin = decToBinary(MemoryInputdec, PagesbitSize + PagesizebitSize);
				InputPageNO = stoi(MemoryInputBin.substr(0, PagesbitSize), nullptr, 2);
				InputPageOffset = stoi(MemoryInputBin.substr(PagesbitSize , MemoryInputBin.length()), nullptr, 2);
			}
			else {
				cout << "Input Virtual address is invalid."; break;
			}
		}


		//--Searching TLB table for already existing pairs
		for(int i=0;i<TLBenteries-1;i++) {
			if (!TLBtable[i].VirtualAddress.compare(MemoryInputBin.substr(0, PagesbitSize))) {
				outputBin = TLBtable[i].PhysicalAddress + decToBinary(InputPageOffset,PagesizebitSize);
				outputDec = stoi(outputBin, nullptr, 2);
				TLBhit = 1;
				TLBhitNumber++;
				break;
			}
		}



		//--Searching in Page table
		if (TLBhit == 0) {
			for (int i = 0; i < NumberOfPages; i++) {
				if (i== InputPageNO && Pagetable[i].validbit == 1) {
					outputBin = decToBinary(Pagetable[i].FrameNO, PagesbitSize) + decToBinary(InputPageOffset, PagesbitSize);
					outputDec = stoi(outputBin, nullptr, 2);


					// Updating TLB
					TLBtable[currentTLBtableindex].VirtualAddress = MemoryInputBin.substr(0, PagesbitSize);
					TLBtable[currentTLBtableindex].PhysicalAddress = outputBin.substr(0, PagesbitSize);
					currentTLBtableindex = (currentTLBtableindex + 1) % (TLBenteries-1);
					break;
				}

				// Page fault Handling
				else if (i == InputPageNO) {

			        // Reading from BACKING_STORE.bin
					if (fseek(MemoryFile, InputPageNO*PageSize, SEEK_SET) != 0) {
						cout << "Backing store is invalid";
						return 0;
					}

					// Writing from buffer into the physical Memory
					fread(buffer, sizeof(signed char), PageSize, MemoryFile);
					for (int j = 0; j < PageSize; j++) {
						PhysicalMemory[currentFreeFrameindex*PageSize + j] = buffer[j];
					}

					// Assigning a free frame using FIFO
					Pagetable[i].FrameNO = currentFreeFrameindex;
					currentFreeFrameindex = (currentFreeFrameindex + 1) % (NumberOfFrames - 1);  //updates the free frame counter according to FIFO
					Pagetable[i].validbit = 1;                                                   
					PageFaultNumber++;
					i--;
					continue;
				}
			}
		}
		outputFile <<"Virtual: "<< MemoryInputdec << "    " <<"Physical: " <<outputDec << "     " <<"Value: "<< (int)PhysicalMemory[Pagetable[InputPageNO].FrameNO*PageSize + InputPageOffset] << endl;		
	}
	outputFile << "Number of addresses: " << InputAddressAmount << endl;
	outputFile << "Number of faults:" << PageFaultNumber << endl;
	outputFile << "Fault rate:" << (double)PageFaultNumber / (double)InputAddressAmount << endl;
	outputFile << "Number of hits:" << TLBhitNumber << endl;
	outputFile << "TLB hit rate:" << (double)TLBhitNumber / (double)InputAddressAmount << endl;
	inputFile.close();
	outputFile.close();
	
	return 0;
	
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
