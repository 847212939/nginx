#pragma once

#include <stddef.h>  //NULL

class CNgx_cpp_crc32
{
private:
	CNgx_cpp_crc32();
public:
	~CNgx_cpp_crc32();
private:
	static CNgx_cpp_crc32 *m_instance;

public:	
	static CNgx_cpp_crc32* GetInstance() 
	{
		if(m_instance == NULL)
		{
			//锁
			if(m_instance == NULL)
			{				
				m_instance = new CNgx_cpp_crc32();
				static CGarhuishou cl; 
			}
			//放锁
		}
		return m_instance;
	}	
	class CGarhuishou 
	{
	public:				
		~CGarhuishou()
		{
			if (CNgx_cpp_crc32::m_instance)
			{						
				delete CNgx_cpp_crc32::m_instance;
				CNgx_cpp_crc32::m_instance = NULL;				
			}
		}
	};
	//-------
public:

	void  Init_CRC32_Table();
	//unsigned long Reflect(unsigned long ref, char ch); // Reflects CRC bits in the lookup table
    unsigned int Reflect(unsigned int ref, char ch); // Reflects CRC bits in the lookup table
    
	//int   Get_CRC(unsigned char* buffer, unsigned long dwSize);
    int   Get_CRC(unsigned char* buffer, unsigned int dwSize);
    
public:
	//unsigned long crc32_table[256]; // Lookup table arrays
    unsigned int crc32_table[256]; // Lookup table arrays
};



