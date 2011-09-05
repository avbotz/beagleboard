#include "stdafx.h"


extern bool   WriteCommBlock (int pComdDev, char *pBuffer ,  int BytesToWrite);
extern void   ReceiveData(int pComDev, char * pBuffer, int BytesToReceive);

/******************************************************************************/
mem_cMemRow::mem_cMemRow(eType Type, unsigned int StartAddr, int RowNumber, eFamily Family)
{
	int Size;



	m_RowNumber = RowNumber;
	m_eFamily    = Family;
	m_eType     = Type;
	m_bEmpty    = TRUE;
	

	if(m_eType == Program)
	{
		if(m_eFamily == dsPIC30F)
		{
			m_RowSize = PM30F_ROW_SIZE;
		}
		else
		{
			m_RowSize = PM33F_ROW_SIZE;
		}
	}
	else
	{
		m_RowSize = EE30F_ROW_SIZE;
	}

	if(m_eType == Program)
	{
		Size = m_RowSize * 3;
		m_Address = StartAddr + RowNumber * m_RowSize * 2;
	}
	if(m_eType == EEProm)
	{
		Size = m_RowSize * 2;
		m_Address = StartAddr + RowNumber * m_RowSize * 2;
	}
	if(m_eType == Configuration)
	{
		Size = 3;
		m_Address = StartAddr + RowNumber * 2;
	}

	m_pBuffer   = (char *)malloc(Size);

	memset(m_Data, 0xFFFF, sizeof(unsigned short)*PM33F_ROW_SIZE*2);	
}
/******************************************************************************/
bool mem_cMemRow::InsertData(unsigned int Address, char * pData)
{
	if(Address < m_Address)
	{
		return FALSE;
	}

	if((m_eType == Program) && (Address >= (m_Address + m_RowSize * 2)))
	{
		return FALSE;
	}

	if((m_eType == EEProm) && (Address >= (m_Address + m_RowSize * 2)))
	{
		return FALSE;
	}

	if((m_eType == Configuration) && (Address >= (m_Address + 2)))
	{
		return FALSE;
	}

	m_bEmpty    = FALSE;

	sscanf(pData, "%4hx", &(m_Data[Address - m_Address]));
	
	return TRUE;
}
/******************************************************************************/
void mem_cMemRow::FormatData(void)
{
	if(m_bEmpty == TRUE)
	{
		return;
	}

	if(m_eType == Program)
	{
		for(int Count = 0; Count < m_RowSize; Count += 1)
		{
			m_pBuffer[0 + Count * 3] = (m_Data[Count * 2]     >> 8) & 0xFF;
			m_pBuffer[1 + Count * 3] = (m_Data[Count * 2])          & 0xFF;
			m_pBuffer[2 + Count * 3] = (m_Data[Count * 2 + 1] >> 8) & 0xFF;
		}
	}
	else if(m_eType == Configuration)
	{
		m_pBuffer[0] = (m_Data[0]  >> 8) & 0xFF;
		m_pBuffer[1] = (m_Data[0])       & 0xFF;
		m_pBuffer[2] = (m_Data[1]  >> 8) & 0xFF;
	}
	else
	{
		for(int Count = 0; Count < m_RowSize; Count++)
		{
			m_pBuffer[0 + Count * 2] = (m_Data[Count * 2] >> 8) & 0xFF;
			m_pBuffer[1 + Count * 2] = (m_Data[Count * 2])      & 0xFF;
		}
	}
}
/******************************************************************************/
void mem_cMemRow::SendData(int pComDev)
{
	char Buffer[4] = {0,0,0,0};

	if((m_bEmpty == TRUE) && (m_eType != Configuration))
	{
		return;
	}

	while(Buffer[0] != COMMAND_ACK)
	{
		if(m_eType == Program)
		{
			Buffer[0] = COMMAND_WRITE_PM;
			Buffer[1] = (m_Address)       & 0xFF;
			Buffer[2] = (m_Address >> 8)  & 0xFF;
			Buffer[3] = (m_Address >> 16) & 0xFF;

			WriteCommBlock(pComDev, Buffer, 4);
			WriteCommBlock(pComDev, m_pBuffer, m_RowSize);
			msleep(10);
			WriteCommBlock(pComDev, m_pBuffer+m_RowSize, m_RowSize);
			msleep(10);
			WriteCommBlock(pComDev, m_pBuffer+m_RowSize+m_RowSize, m_RowSize);
			msleep(10);
		}
		else if((m_eType == Configuration) && (m_RowNumber == 0))
		{
			Buffer[0] = COMMAND_WRITE_CM;
			Buffer[1] = (char)(m_bEmpty)& 0xFF;
			Buffer[2] = m_pBuffer[0];
			Buffer[3] = m_pBuffer[1];

			WriteCommBlock(pComDev, Buffer, 4);
			
		}
		else if((m_eType == Configuration) && (m_RowNumber != 0))
		{
			if((m_eFamily == dsPIC30F) && (m_RowNumber == 7))
			{
				return;
			}

			Buffer[0] = (char)(m_bEmpty)& 0xFF;
			Buffer[1] = m_pBuffer[0];
			Buffer[2] = m_pBuffer[1];

			WriteCommBlock(pComDev, Buffer, 3);

		}

		else
		{
			assert(!"Unknown memory type");
		}

		ReceiveData(pComDev, Buffer, 1);
	}

}
