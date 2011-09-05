#ifndef _mem_h
#define _mem_h

class mem_cMemRow
{
public:

	enum eType
	{
		Program,
		EEProm,
		Configuration
	};
	mem_cMemRow(eType Type, unsigned int StartAddr, int RowNumber, eFamily Family);

	bool InsertData(unsigned int Address, char * pData);
	void FormatData(void);
	void SendData  (int pComDev);

private:
	char           * m_pBuffer;
	unsigned int     m_Address;
	bool             m_bEmpty;
	eType            m_eType;
	unsigned short   m_Data[PM33F_ROW_SIZE*2];
	int              m_RowNumber;
	eFamily          m_eFamily;
	int				 m_RowSize;
};


#endif
