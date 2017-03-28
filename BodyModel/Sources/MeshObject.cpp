#include "pch.h"
#include "MeshObject.h"
#include "OpenGEX/OpenGEX.h"

#include <Kore/IO/FileReader.h>

using namespace Kore;

MeshObject::MeshObject(const char* filename) {
	FileReader fileReader(filename, FileReader::Asset);
	void* data = fileReader.readAll();
	int size = fileReader.size() + 1;
	char* buffer = new char[size + 1];
	for (int i = 0; i < size; ++i) buffer[i] = reinterpret_cast<char*>(data)[i];
	buffer[size] = 0;
	
	//Mesh* mesh = new Mesh;
	
	OGEX::OpenGexDataDescription openGexDataDescription;
	DataResult result = openGexDataDescription.ProcessText(buffer);
	if (result == kDataOkay) {
		const Structure* structure = openGexDataDescription.GetRootStructure()->GetFirstSubnode();
		while (structure) {
			// This loops over all top-level structures in the file.
			
			// Do something with the data...
			
			structure = structure->Next();
		}
	}
	
	delete[] buffer;
	
}

/*
int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int cmdShow)
{
	// Import the file "Code/Example.ogex".
 
	HANDLE handle = CreateFile("Code\\Example.ogex", GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (handle != INVALID_HANDLE_VALUE)
	{
		OpenGexDataDescription	openGexDataDescription;
		DWORD					actual;
		
		DWORD size = GetFileSize(handle, nullptr);
		char *buffer = new char[size + 1];
		
		// Read the entire contents of the file and put a zero terminator at the end.
		
		ReadFile(handle, buffer, size, &actual, nullptr);
		buffer[size] = 0;
		
		// Once the file is in memory, the DataDescription::ProcessText() function
		// is called to create the structure tree and process the data.
		
		DataResult result = openGexDataDescription.ProcessText(buffer);
		if (result == kDataOkay)
		{
			const Structure *structure = openGexDataDescription.GetRootStructure()->GetFirstSubnode();
			while (structure)
			{
				// This loops over all top-level structures in the file.
				
				// Do something with the data...
				
				structure = structure->Next();
			}
		}
		
		delete[] buffer;
	}
 
	return (0);
}
*/
