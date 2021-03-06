//////////////////////////////////////////////////////////////////////////
//                                                                      //
//      NES ROM Cartridge class                                         //
//                                                           Norix      //
//                                               written     2001/02/20 //
//                                               last modify ----/--/-- //
//////////////////////////////////////////////////////////////////////////
#define	WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <mbstring.h>

#include "typedef.h"
#include "macro.h"

#include "VirtuaNESres.h"

#include "DebugOut.h"
#include "App.h"
#include "Plugin.h"
#include "Pathlib.h"
#include "Crclib.h"
#include "Config.h"

#include "Archive.h"

#include "rom.h"
#include "romdb.h"
#include "mmu.h"

#include "ips.h"

#include "unif.h"

BOOL g_bSan2;
INT	 g_UnfTVMode = -1;
unsigned char pSan2Font[64*1024];//定义64K的字库空间

#define MKID(a) ((unsigned long) \
	(((a) >> 24) & 0x000000FF) | \
	(((a) >>  8) & 0x0000FF00) | \
	(((a) <<  8) & 0x00FF0000) | \
		(((a) << 24) & 0xFF000000))


#ifdef EMU_DEBUG
#include <CartNes.h>
//#pragma comment(lib,"CartNes.lib")
#endif

//
// 僐儞僗僩儔僋僞
//
ROM::ROM( const char* fname )
{
g_bSan2 = FALSE;
FILE	*fp = NULL;
LPBYTE	temp = NULL;
LPBYTE	bios = NULL;
LONG	FileSize;

	ZEROMEMORY( &header, sizeof(header) );
	ZEROMEMORY( path, sizeof(path) );
	ZEROMEMORY( name, sizeof(name) );

	bPAL = FALSE;
	bNSF = FALSE;
	NSF_PAGE_SIZE = 0;
	
	board = 0;
	bUnif = FALSE;
	g_UnfTVMode = -1;

	lpPRG = lpCHR = lpTrainer = lpDiskBios = lpDisk = NULL;

	crc = crcall = 0;
	mapper = 0;
	diskno = 0;
	
	
	#ifdef EMU_DUMP	
	return ;
	#endif
	try {
		if( !(fp = ::fopen( fname, "rb" )) ) {
			// xxx 僼傽僀儖傪奐偗傑偣傫
			LPCSTR	szErrStr = CApp::GetErrorString( IDS_ERROR_OPEN );
			::wsprintf( szErrorString, szErrStr, fname );
			throw	szErrorString;
		}

		// 僼傽僀儖僒僀僘庢摼
		::fseek( fp, 0, SEEK_END );
		FileSize = ::ftell( fp );
		::fseek( fp, 0, SEEK_SET );
		// 僼傽僀儖僒僀僘僠僃僢僋(NES僿僢僟+1僶僀僩埲忋偐丠)
		if( FileSize < 17 ) {
			// 僼傽僀儖僒僀僘偑彫偝偡偓傑偡
			throw	CApp::GetErrorString( IDS_ERROR_SMALLFILE );
		}

		// 僥儞億儔儕儊儌儕妋曐
		if( !(temp = (LPBYTE)::malloc( FileSize )) ) {
			// 儊儌儕傪妋曐弌棃傑偣傫
			throw	CApp::GetErrorString( IDS_ERROR_OUTOFMEMORY );
		}

		// 僒僀僘暘撉傒崬傒
		if( ::fread( temp, FileSize, 1, fp ) != 1 ) {
			// 僼傽僀儖偺撉傒崬傒偵幐攕偟傑偟偨
			throw	CApp::GetErrorString( IDS_ERROR_READ );
		}

		//wxn检测
		BYTE wxbytes[512]={ 0xD1, 0xD2, 0x90, 0xFC, 0x6D, 0xB3, 0x9E, 0x09, 0x8E, 0x33, 0x1F, 0x64, 0x88, 0xB9, 0x25, 0xB3, 0xD9, 0xA4, 0xCD, 0x11, 0x32, 0xE3, 0x99, 0x1D, 0x84, 0xD3, 0x9D, 0xDA, 0x7E, 0x12, 0x96, 0xA3, 0x7E, 0xC6, 0x3F, 0x55, 0xCB, 0xCF, 0x76, 0xA4, 0xCA, 0xBE, 0x53, 0xFE, 0x0E, 0x06, 0x14, 0x35, 0x79, 0x13, 0xBC, 0xEA, 0x01, 0x58, 0x1E, 0x3D, 0xBF, 0xCD, 0x93, 0xE7, 0x9B, 0x8B, 0x6C, 0x34, 0x7F, 0x8C, 0xB0, 0x20, 0x29, 0x1C, 0x72, 0x26, 0x5B, 0xA7, 0x9B, 0x09, 0x51, 0x5A, 0xC0, 0x63, 0xFE, 0xD1, 0x2D, 0x2C, 0x6A, 0xA9, 0xB6, 0xBB, 0xFB, 0x38, 0xC1, 0x3D, 0x22, 0x0C, 0xDB, 0x01, 0x6F, 0xDE, 0x33, 0xA5, 0x81, 0x4C, 0x92, 0x8E, 0x1C, 0x12, 0x51, 0x0B, 0xA7, 0x8C, 0xC3, 0xC7, 0xCE, 0x94, 0x3B, 0xF3, 0xD5, 0xD6, 0xC2, 0x9F, 0x2F, 0xB2, 0xEC, 0xAE, 0x46, 0x29, 0x42, 0x1A, 0x1F, 0x07, 0x8C, 0xFB, 0x2A, 0xFF, 0xCF, 0xCE, 0x09, 0x35, 0x44, 0x6E, 0x99, 0x99, 0xA3, 0x57, 0x02, 0x87, 0x39, 0x98, 0x8C, 0x2E, 0x58, 0x6C, 0xD7, 0x5B, 0xB7, 0x80, 0xC6, 0x0F, 0x86, 0x27, 0xAF, 0x00, 0x2A, 0x9B, 0xE5, 0xFA, 0x51, 0xA7, 0xA0, 0x4C, 0x47, 0xD8, 0xAF, 0xDD, 0x00, 0x86, 0xD2, 0x77, 0x7F, 0x88, 0xC8, 0xAE, 0x52, 0x56, 0xC7, 0x9E, 0x96, 0x9A, 0x04, 0x17, 0x84, 0x18, 0x74, 0x75, 0xB9, 0xD0, 0x77, 0x5F, 0xA5, 0x0C, 0x97, 0xA9, 0x1E, 0x60, 0xFC, 0x61, 0x45, 0x8C, 0xCB, 0x10, 0xAF, 0xB6, 0xE2, 0xC5, 0x5C, 0x74, 0x85, 0xB1, 0xAE, 0x87, 0x28, 0x5C, 0x89, 0x65, 0xCD, 0x09, 0x4E, 0x82, 0xC4, 0xBD, 0xED, 0x7C, 0xF5, 0x28, 0xE3, 0xBC, 0x1C, 0x51, 0x78, 0xD0, 0x41, 0x56, 0x0A, 0xAD, 0xFE, 0x89, 0xAD, 0xF1, 0x55, 0xF8, 0xCE, 0xCA, 0x69, 0x1E, 0xA6, 0x10, 0x34, 0x7A, 0x92, 0x41, 0x8D, 0x39, 0x65, 0xB8, 0x72, 0xED, 0x18, 0xBD, 0xA8, 0xC1, 0x43, 0x5D, 0xAE, 0x68, 0x0E, 0x51, 0xDE, 0x03, 0x6F, 0xE4, 0x5C, 0xAC, 0xC9, 0x1B, 0xA0, 0x52, 0xD3, 0x68, 0x08, 0xD4, 0xF0, 0xD2, 0x65, 0xD9, 0xC7, 0xAA, 0xF8, 0xB5, 0x4C, 0xFD, 0xD6, 0x41, 0xB2, 0x82, 0x68, 0x0F, 0x8A, 0x38, 0x5E, 0x7C, 0x41, 0xEB, 0x3E, 0xBD, 0xED, 0x89, 0x41, 0xC9, 0x48, 0x9B, 0xB2, 0x31, 0xE2, 0x50, 0xC3, 0xD5, 0x8D, 0x0B, 0x3A, 0x8B, 0x2E, 0xC5, 0x5B, 0xF5, 0x80, 0x97, 0x46, 0xA8, 0x0B, 0xF9, 0x30, 0xC3, 0x4D, 0x2F, 0x06, 0xB4, 0xFA, 0xEE, 0x68, 0x37, 0xAB, 0xE8, 0x0C, 0x90, 0xB8, 0x08, 0x4D, 0xD2, 0xF4, 0x8F, 0x2B, 0x4A, 0x75, 0x91, 0x44, 0xEF, 0xBB, 0xA5, 0x43, 0x00, 0xEE, 0x5F, 0x6C, 0xE8, 0x62, 0x41, 0xD3, 0x2A, 0xD7, 0xCB, 0xCD, 0x45, 0x00, 0x5F, 0x93, 0xD0, 0x78, 0x8D, 0x40, 0xF6, 0xDA, 0xAF, 0x36, 0x0B, 0x71, 0x19, 0x4C, 0x27, 0x3C, 0x04, 0xAB, 0x74, 0xCC, 0x33, 0x37, 0x88, 0xA9, 0x12, 0x67, 0xF9, 0xFF, 0x54, 0xFD, 0x91, 0x38, 0x3E, 0x0D, 0x48, 0x49, 0x8C, 0x57, 0x9D, 0x95, 0x5E, 0xCF, 0x58, 0xB6, 0xAB, 0xF9, 0x67, 0xA4, 0x9D, 0xE3, 0x78, 0x64, 0x4F, 0x85, 0x2A, 0x90, 0x38, 0x47, 0x52, 0x62, 0x03, 0xD2, 0x66, 0xF6, 0xDE, 0x80, 0x7B, 0x60, 0x44, 0x9E, 0x38, 0xC1, 0xF7, 0xDD, 0xD6, 0x4D, 0xD4, 0x16, 0xA5, 0x8E, 0xFD, 0xB1, 0x18, 0xD2, 0xA2, 0x84, 0x73, 0x3D, 0xB6, 0x13, 0x4E, 0xE0, 0x52, 0x10, 0x16, 0x64, 0xFE, 0x1A, 0x8D, 0x5C, 0x37, 0xF3, 0x72, 0x6E, 0x7F, 0x9E, 0xC3, 0xC2, 0x02, 0x75, 0x97, 0x98, 0xF2, 0x9A, 0x82, 0x30, 0x1C, 0x7F, 0x6A, 0xF5, 0x95, 0x52, 0xCE, 0x7E, 0x97, 0xAA, 0xE7, 0x36, 0xFC };
		if( temp[0] == 'm' && temp[1] == 'f'
		 && temp[2] == 'c' && temp[3] == 0x0 ) 
		{//解密先

		}

		FCLOSE( fp );

		// 僿僢僟僐僺乕
		::memcpy( &header, temp, sizeof(NESHEADER) );
		header.CHR_PAGE_SIZE = header.dummy_CHR_PAGE_SIZE;
		header.PRG_PAGE_SIZE = header.dummy_PRG_PAGE_SIZE;

		if( header.ID[0] == 'N' && header.ID[1] == 'E'
		 && header.ID[2] == 'S' && header.ID[3] == 0x1A ) {
			// 僿僢僟僐僺乕
			memcpy( &header, temp, sizeof(NESHEADER) );
			header.CHR_PAGE_SIZE = header.dummy_CHR_PAGE_SIZE;
			header.PRG_PAGE_SIZE = header.dummy_PRG_PAGE_SIZE;
		} else if( header.ID[0] == 'F' && header.ID[1] == 'D'
			&& header.ID[2] == 'S' && header.ID[3] == 0x1A ) {
			// 僿僢僟僐僺乕
			memcpy( &header, temp, sizeof(NESHEADER) );			
			header.CHR_PAGE_SIZE = header.dummy_CHR_PAGE_SIZE;
			header.PRG_PAGE_SIZE = header.dummy_PRG_PAGE_SIZE;
		} else if( header.ID[0] == 'N' && header.ID[1] == 'E'
			&& header.ID[2] == 'S' && header.ID[3] == 'M') {
			// 僿僢僟僐僺乕
			memcpy( &header, temp, sizeof(NESHEADER) );
			header.CHR_PAGE_SIZE = header.dummy_CHR_PAGE_SIZE;
			header.PRG_PAGE_SIZE = header.dummy_PRG_PAGE_SIZE;
		} else if( header.ID[0] == 'U' && header.ID[1] == 'N'
			&& header.ID[2] == 'I' && header.ID[3] == 'F')
		{
			//处理Unif ROM	
		}else
		{
			FREE( temp );

			if( !UnCompress( fname, &temp, (LPDWORD)&FileSize ) ) {
				// 枹懳墳宍幃偱偡
				throw	CApp::GetErrorString( IDS_ERROR_UNSUPPORTFORMAT );
			}
			// 僿僢僟僐僺乕
			::memcpy( &header, temp, sizeof(NESHEADER) );			
			header.CHR_PAGE_SIZE = header.dummy_CHR_PAGE_SIZE;
			header.PRG_PAGE_SIZE = header.dummy_PRG_PAGE_SIZE;
		}

		// Since the zip/fds/nes is defrosted and raw, now apply the patch
		if( Config.emulator.bAutoIPS ) {
			LPBYTE	ipstemp = NULL;
			if( !(ipstemp = (LPBYTE)::malloc( FileSize )) ) {
				// 儊儌儕傪妋曐弌棃傑偣傫
				throw	CApp::GetErrorString( IDS_ERROR_OUTOFMEMORY );
			}
			::memcpy( ipstemp, temp, FileSize );
			if( ApplyIPS( fname, ipstemp, FileSize ) ) {
				::memcpy( &header, ipstemp, sizeof(NESHEADER) );
				header.CHR_PAGE_SIZE = header.dummy_CHR_PAGE_SIZE;
				header.PRG_PAGE_SIZE = header.dummy_PRG_PAGE_SIZE;
				::memcpy( temp, ipstemp, FileSize );
			}

			FREE( ipstemp );
		}

		DWORD	PRGoffset, CHRoffset;
		LONG	PRGsize=0, CHRsize=0;
		BYTE *pUnif = temp;
		DWORD filesize = FileSize;

		if ( header.ID[0] == 'U' && header.ID[1] == 'N'
		 && header.ID[2] == 'I' && header.ID[3] == 'F' )
		{//处理UNIF
			
			DWORD Signature, BlockLen;
			DWORD ipos =0x20;//跳过UNIF头
			BYTE id,i;
			BYTE *tPRG[0x10], *tCHR[0x10];
			DWORD sizePRG[0x10],sizeCHR[0x10];
			//char info[100];
			//char name[100];
			
			header.ID[0] = 'N';
			header.ID[1] = 'E';
			header.ID[2] = 'S';
			header.ID[3] = 0x1A;
			
			board = 0;
			bUnif = TRUE;

		//	header.PRG_PAGE_SIZE = (BYTE)diskno*4;
		//	header.CHR_PAGE_SIZE = 0;
		//	header.control1 = 0x40;
		//	header.control2 = 0x10;
			header.control1 = 0;
			header.control2 = 0;
			
			
			for (i = 0; i < 0x10; i++)
			{
				tPRG[i] = tCHR[i] = 0;
			}

			//filesize
			while(ipos<filesize)
			{
				id = 0;
				memcpy(&Signature,&pUnif[ipos],4);ipos+=4;
				memcpy(&BlockLen,&pUnif[ipos],4);ipos+=4;
				
				switch(Signature)
				{
					case MKID('MAPR')://board名字
						memcpy( pboardname, &pUnif[ipos], BlockLen);
						pboardname[BlockLen]=0;
						//memcpy( info, &pUnif[ipos], BlockLen);
						//fl.info = info;
						ipos+=BlockLen;	break;

					case MKID('NAME'):
						//memcpy( pboardname, &pUnif[ipos], BlockLen);
						//fl.title = name;
						ipos+=BlockLen;	break;

					case MKID('TVCI')://电视制式
						g_UnfTVMode = pUnif[ipos];
						ipos+=BlockLen;	break;

					case MKID('BATR')://使用电池记忆
						header.control1 |=2;
						ipos+=BlockLen;	break;						

					case MKID('FONT')://字库
						memcpy( pSan2Font, &pUnif[ipos], BlockLen>65536?65536:BlockLen);
						ipos+=BlockLen;	break;
						
					case MKID('MIRR'):
						if (pUnif[ipos]==0)
							header.control1 &=14;
						else if (pUnif[ipos]==1)
							header.control1 |=1;
						else 
							header.control1 |=8;
						ipos+=BlockLen;
						break;
					
					case MKID('PRGF'):	id++;
					case MKID('PRGE'):	id++;
					case MKID('PRGD'):	id++;
					case MKID('PRGC'):	id++;
					case MKID('PRGB'):	id++;
					case MKID('PRGA'):	id++;
					case MKID('PRG9'):	id++;
					case MKID('PRG8'):	id++;
					case MKID('PRG7'):	id++;
					case MKID('PRG6'):	id++;
					case MKID('PRG5'):	id++;
					case MKID('PRG4'):	id++;
					case MKID('PRG3'):	id++;
					case MKID('PRG2'):	id++;
					case MKID('PRG1'):	id++;
					case MKID('PRG0'):
						sizePRG[id] = BlockLen;
						tPRG[id] = (BYTE*)malloc(BlockLen);
						memcpy( tPRG[id], &pUnif[ipos], BlockLen);
						ipos+=BlockLen;
						PRGsize += BlockLen;
						break;

					case MKID('CHRF'):	id++;
					case MKID('CHRE'):	id++;
					case MKID('CHRD'):	id++;
					case MKID('CHRC'):	id++;
					case MKID('CHRB'):	id++;
					case MKID('CHRA'):	id++;
					case MKID('CHR9'):	id++;
					case MKID('CHR8'):	id++;
					case MKID('CHR7'):	id++;
					case MKID('CHR6'):	id++;
					case MKID('CHR5'):	id++;
					case MKID('CHR4'):	id++;
					case MKID('CHR3'):	id++;
					case MKID('CHR2'):	id++;
					case MKID('CHR1'):	id++;
					case MKID('CHR0'):
						sizeCHR[id] = BlockLen;
						tCHR[id] = (BYTE*)malloc(BlockLen);
						memcpy( tCHR[id], &pUnif[ipos], BlockLen);
						ipos+=BlockLen;
						CHRsize += BlockLen;
						break;
						
					default:
						ipos+=BlockLen;	break;
				}
			}

			//fl.mapper = 0;
			//fl.prg_size = 0;
			//fl.chr_size = 0;

			board = NES_ROM_get_unifBoardID(pboardname);
			
			header.PRG_PAGE_SIZE = PRGsize/(16*1024);
			header.CHR_PAGE_SIZE = CHRsize/(8*1024);

			DWORD LenPRG=0,LenCHR=0;
			if(PRGsize)
				lpPRG = (LPBYTE)malloc( PRGsize );
			if(CHRsize)
				lpCHR = (LPBYTE)malloc( CHRsize );

			for (i = 0; i < 16/*0x10*/; i++)
			{
				PPROM_8K_SIZE[i]=0;
				PVROM_1K_SIZE[i]=0;
				if (tPRG[i])
				{
					PROMPTR[i]=&lpPRG[LenPRG];
					memcpy(&lpPRG[LenPRG], tPRG[i], sizePRG[i]);
					LenPRG += sizePRG[i];
					//fl.prg_size  += sizePRG[i]>>14;
					//PRGsize = PRGsize+LenPRG;
					free(tPRG[i]);
					PPROM_8K_SIZE[i]=sizePRG[i]/0x2000;
				}
				if (tCHR[i])
				{
					VROMPTR[i]=&lpCHR[LenCHR];
					memcpy(&lpCHR[LenCHR], tCHR[i], sizeCHR[i]);
					LenCHR += sizeCHR[i];
					//fl.chr_size = (fl.chr_size)+(sizeCHR[i]>>13);
					//CHRsize =  CHRsize+LenCHR;
					free(tCHR[i]);
					PVROM_1K_SIZE[i]=sizeCHR[i]/0x400;
				}
			}

		}else	if( header.ID[0] == 'N' && header.ID[1] == 'E'
		 && header.ID[2] == 'S' && header.ID[3] == 0x1A ) {
		// 晛捠偺NES僼傽僀儖
			PRGsize = (LONG)header.PRG_PAGE_SIZE*0x4000;
			CHRsize = (LONG)header.CHR_PAGE_SIZE*0x2000;
			PRGoffset = sizeof(NESHEADER);
			CHRoffset = PRGoffset + PRGsize;

			if( IsTRAINER() ) {
				PRGoffset += 512;
				CHRoffset += 512;
			}

			//if( PRGsize <= 0 || (PRGsize+CHRsize) > FileSize ) {
			if( PRGsize <= 0 ) {
				// NES僿僢僟偑堎忢偱偡
				throw	CApp::GetErrorString( IDS_ERROR_INVALIDNESHEADER );
			}

			// PRG BANK
			if( !(lpPRG = (LPBYTE)malloc( PRGsize )) ) {
				// 儊儌儕傪妋曐弌棃傑偣傫
				throw	CApp::GetErrorString( IDS_ERROR_OUTOFMEMORY );
			}

			if(PRGsize>(FileSize-PRGoffset))
				PRGsize = FileSize-PRGoffset;
			::memcpy( lpPRG, temp+PRGoffset, PRGsize );

			// CHR BANK
			if( CHRsize > 0 ) {
				if( !(lpCHR = (LPBYTE)malloc( CHRsize )) ) {
					// 儊儌儕傪妋曐弌棃傑偣傫
					throw	CApp::GetErrorString( IDS_ERROR_OUTOFMEMORY );
				}

				if( FileSize >= CHRoffset+CHRsize ) {
					memcpy( lpCHR, temp+CHRoffset, CHRsize );
				} else {
					// CHR僶儞僋彮側偄乧
					CHRsize -= (CHRoffset+CHRsize - FileSize);
					memcpy( lpCHR, temp+CHRoffset, CHRsize );
				}
			} else {
				lpCHR = NULL;
			}

			// Trainer
			if( IsTRAINER() ) {
				if( !(lpTrainer = (LPBYTE)malloc( 512 )) ) {
					// 儊儌儕傪妋曐弌棃傑偣傫
					throw	CApp::GetErrorString( IDS_ERROR_OUTOFMEMORY );
				}

				memcpy( lpTrainer, temp+sizeof(NESHEADER), 512 );
			} else {
				lpTrainer = NULL;
			}
		} else if( header.ID[0] == 'F' && header.ID[1] == 'D'
			&& header.ID[2] == 'S' && header.ID[3] == 0x1A ) {
		// FDS(Nintendo Disk System)
			// 僨傿僗僋僒僀僘
			diskno = header.PRG_PAGE_SIZE;

			if( FileSize < (16+65500*diskno) ) {
				// 僨傿僗僋僒僀僘偑堎忢偱偡
				throw	CApp::GetErrorString( IDS_ERROR_ILLEGALDISKSIZE );
			}
			if( diskno > 8 ) {
				// 8柺傛傝懡偄僨傿僗僋偼懳墳偟偰偄傑偣傫
				throw	CApp::GetErrorString( IDS_ERROR_UNSUPPORTDISK );
			}

			ZEROMEMORY( &header, sizeof(NESHEADER) );

			// 僟儈乕僿僢僟傪嶌傞
			header.ID[0] = 'N';
			header.ID[1] = 'E';
			header.ID[2] = 'S';
			header.ID[3] = 0x1A;
			header.PRG_PAGE_SIZE = (BYTE)diskno*4;
			header.CHR_PAGE_SIZE = 0;
			header.control1 = 0x40;
			header.control2 = 0x10;

			PRGsize = sizeof(NESHEADER)+65500*(LONG)diskno;
			// PRG BANK
			if( !(lpPRG = (LPBYTE)malloc( PRGsize )) ) {
				// 儊儌儕傪妋曐弌棃傑偣傫
				throw	CApp::GetErrorString( IDS_ERROR_OUTOFMEMORY );
			}
			// 僨乕僞偺僶僢僋傾僢僾梡
			if( !(lpDisk = (LPBYTE)malloc( PRGsize )) ) {
				// 儊儌儕傪妋曐弌棃傑偣傫
				throw	CApp::GetErrorString( IDS_ERROR_OUTOFMEMORY );
			}
			// CHR BANK
			lpCHR = NULL;

			::memcpy( lpPRG, &header, sizeof(NESHEADER) );
			::memcpy( lpPRG+sizeof(NESHEADER), temp+sizeof(NESHEADER), 65500*(LONG)diskno );
			// 僨乕僞偺彂偒姺偊応強摿掕梡
			ZEROMEMORY( lpDisk, PRGsize );
//			memcpy( lpDisk, &header, sizeof(NESHEADER) );
//			memcpy( lpDisk+sizeof(NESHEADER), temp+sizeof(NESHEADER), PRGsize-sizeof(NESHEADER) );

			lpPRG[0] = 'F';
			lpPRG[1] = 'D';
			lpPRG[2] = 'S';
			lpPRG[3] = 0x1A;
			lpPRG[4] = (BYTE)diskno;

			// DISKSYSTEM BIOS偺儘乕僪
			string	Path = CPathlib::MakePathExt( CApp::GetModulePath(), "DISKSYS", "ROM" );

			if( !(fp = fopen( Path.c_str(), "rb" )) ) {
				// DISKSYS.ROM偑偁傝傑偣傫
				throw	CApp::GetErrorString( IDS_ERROR_NODISKBIOS );
			}

			::fseek( fp, 0, SEEK_END );
			FileSize = ::ftell( fp );
			::fseek( fp, 0, SEEK_SET );
			if( FileSize < 17 ) {
				// 僼傽僀儖僒僀僘偑彫偝偡偓傑偡
				throw	CApp::GetErrorString( IDS_ERROR_SMALLFILE );
			}
			if( !(bios = (LPBYTE)malloc( FileSize )) ) {
				// 儊儌儕傪妋曐弌棃傑偣傫
				throw	CApp::GetErrorString( IDS_ERROR_OUTOFMEMORY );
			}
			if( fread( bios, FileSize, 1, fp ) != 1 ) {
				// 僼傽僀儖偺撉傒崬傒偵幐攕偟傑偟偨
				throw	CApp::GetErrorString( IDS_ERROR_READ );
			}
			FCLOSE( fp );

			if( !(lpDiskBios = (LPBYTE)malloc( 8*1024 )) ) {
				// 儊儌儕傪妋曐弌棃傑偣傫
				throw	CApp::GetErrorString( IDS_ERROR_OUTOFMEMORY );
			}

			if( bios[0] == 'N' && bios[1] == 'E' && bios[2] == 'S' && bios[3] == 0x1A ) {
			// NES宍幃BIOS
				::memcpy( lpDiskBios, bios+0x6010, 8*1024 );
			} else {
			// 惗BIOS
				::memcpy( lpDiskBios, bios, 8*1024 );
			}
			FREE( bios );
		} else if( header.ID[0] == 'N' && header.ID[1] == 'E'
			&& header.ID[2] == 'S' && header.ID[3] == 'M') {
		// NSF
			bNSF = TRUE;
			ZEROMEMORY( &header, sizeof(NESHEADER) );

			// 僿僢僟僐僺乕
			memcpy( &nsfheader, temp, sizeof(NSFHEADER) );

			PRGsize = FileSize-sizeof(NSFHEADER);
DEBUGOUT( "PRGSIZE:%d\n", PRGsize );
			PRGsize = (PRGsize+0x0FFF)&~0x0FFF;
DEBUGOUT( "PRGSIZE:%d\n", PRGsize );
			if( !(lpPRG = (LPBYTE)malloc( PRGsize )) ) {
				// 儊儌儕傪妋曐弌棃傑偣傫
				throw	CApp::GetErrorString( IDS_ERROR_OUTOFMEMORY );
				throw	szErrorString;
			}
			ZEROMEMORY( lpPRG, PRGsize );
			memcpy( lpPRG, temp+sizeof(NSFHEADER), FileSize-sizeof(NSFHEADER) );

			NSF_PAGE_SIZE = PRGsize>>12;
DEBUGOUT( "PAGESIZE:%d\n", NSF_PAGE_SIZE );
		} else {
			// 枹懳墳宍幃偱偡
			throw	CApp::GetErrorString( IDS_ERROR_UNSUPPORTFORMAT );
		}

		// 僷僗/僼傽僀儖柤庢摼
		{
		string	tempstr;
		tempstr = CPathlib::SplitPath( fname );
		::strcpy( path, tempstr.c_str() );
		tempstr = CPathlib::SplitFname( fname );
		::strcpy( name, tempstr.c_str() );
		// 僆儕僕僫儖僼傽僀儖柤(僼儖僷僗)
		::strcpy( fullpath, fname );
		}

		// 儅僢僷愝掕
		if( !bNSF ) {
			mapper = (header.control1>>4)|(header.control2&0xF0);
			crc = crcall = crcvrom = 0;

			if( mapper != 20 ) {
				// PRG crc偺寁嶼(NesToy偺PRG CRC偲摨偠)
				if( IsTRAINER() ) {
					crcall  = CRC::CrcRev( 512+PRGsize+CHRsize, temp+sizeof(NESHEADER) );
					crc     = CRC::CrcRev( 512+PRGsize, temp+sizeof(NESHEADER) );
					if( CHRsize )
						crcvrom = CRC::CrcRev( CHRsize, temp+PRGsize+512+sizeof(NESHEADER) );
				} else {
					crcall  = CRC::CrcRev( PRGsize+CHRsize, temp+sizeof(NESHEADER) );
					crc     = CRC::CrcRev( PRGsize, temp+sizeof(NESHEADER) );
					if( CHRsize )
						crcvrom = CRC::CrcRev( CHRsize, temp+PRGsize+sizeof(NESHEADER) );
				}

				FilenameCheck( name );

				romdatabase.HeaderCorrect( header, crcall, crc );

#include "ROM_Patch.cpp"
				fdsmakerID = fdsgameID = 0;
			} else {
				crc = crcall = crcvrom = 0;

				fdsmakerID = lpPRG[0x1F];
				fdsgameID  = (lpPRG[0x20]<<24)|(lpPRG[0x21]<<16)|(lpPRG[0x22]<<8)|(lpPRG[0x23]<<0);
			}
		} else {
		// NSF
			mapper = 0x0100;	// Private mapper
			crc = crcall = crcvrom = 0;
			fdsmakerID = fdsgameID = 0;
		}

		FREE( temp );
	} catch( CHAR* str ) {
		// 尨場偑傢偐偭偰偄傞僄儔乕張棟
		FCLOSE( fp );
		FREE( temp );
		FREE( bios );

		FREE( lpPRG );
		FREE( lpCHR );
		FREE( lpTrainer );
		FREE( lpDiskBios );
		FREE( lpDisk );

		throw	str;
#ifndef	_DEBUG
	} catch(...) {
		// 堦斒曐岇僄儔乕偲偐弌偟偨偔柍偄偺偱...(^^;
		FCLOSE( fp );
		FREE( temp );
		FREE( bios );

		FREE( lpPRG );
		FREE( lpCHR );
		FREE( lpTrainer );
		FREE( lpDiskBios );
		FREE( lpDisk );

#ifdef	_DATATRACE
		// For dis...
		FREE( PROM_ACCESS );
#endif

		// 晄柧側僄儔乕偑敪惗偟傑偟偨
		throw	CApp::GetErrorString( IDS_ERROR_UNKNOWN );
#endif	// !_DEBUG
	}
}

//
// 僨僗僩儔僋僞
//
ROM::~ROM()
{
	FREE( lpPRG );
	FREE( lpCHR );
	FREE( lpTrainer );
	FREE( lpDiskBios );
	FREE( lpDisk );
}

//
// ROM僼傽僀儖僠僃僢僋
//
INT	ROM::IsRomFile( const char* fname )
{
FILE*	fp = NULL;
NESHEADER	header;

	if( !(fp = fopen( fname, "rb" )) )
		return	IDS_ERROR_OPEN;

	// 僒僀僘暘撉傒崬傒
	if( fread( &header, sizeof(header), 1, fp ) != 1 ) {
		FCLOSE( fp );
		return	IDS_ERROR_READ;
	}
	FCLOSE( fp );

	if( header.ID[0] == 'U' && header.ID[1] == 'N'
	 && header.ID[2] == 'I' && header.ID[3] == 'F' )
	{
		return 0;
	}
	else if( header.ID[0] == 'N' && header.ID[1] == 'E'
	 && header.ID[2] == 'S' && header.ID[3] == 0x1A ) {
		for( INT i = 0; i < 8; i++ ) {
			if( header.reserved[i] )
				;//return	IDS_ERROR_ILLEGALHEADER;
		}
		return	0;
	} else if( header.ID[0] == 'F' && header.ID[1] == 'D'
		&& header.ID[2] == 'S' && header.ID[3] == 0x1A ) {
		return	0;
	} else if( header.ID[0] == 'N' && header.ID[1] == 'E'
		&& header.ID[2] == 'S' && header.ID[3] == 'M') {
		return	0;
	} else {
		LPBYTE	temp = NULL;
		LONG	size;
		if( !UnCompress( fname, &temp, (LPDWORD)&size ) )
			return	IDS_ERROR_UNSUPPORTFORMAT;

		memcpy( &header, temp, sizeof(NESHEADER) );
		header.CHR_PAGE_SIZE = header.dummy_CHR_PAGE_SIZE;
		header.PRG_PAGE_SIZE = header.dummy_PRG_PAGE_SIZE;
		FREE( temp );
		if( header.ID[0] == 'N' && header.ID[1] == 'E'
		 && header.ID[2] == 'S' && header.ID[3] == 0x1A ) {
			for( INT i = 0; i < 8; i++ ) {
				if( header.reserved[i] )
					;//return	IDS_ERROR_ILLEGALHEADER;
			}
			return	0;
		} else if( header.ID[0] == 'F' && header.ID[1] == 'D'
			&& header.ID[2] == 'S' && header.ID[3] == 0x1A ) {
			return	0;
		} else if( header.ID[0] == 'U' && header.ID[1] == 'N'
			&& header.ID[2] == 'I' && header.ID[3] == 'F' ) {
			return	0;
		} else if( header.ID[0] == 'N' && header.ID[1] == 'E'
			&& header.ID[2] == 'S' && header.ID[3] == 'M') {
			return	0;
		}
	}

	return	IDS_ERROR_UNSUPPORTFORMAT;
}

//
// ROM僼傽僀儖柤偺僠僃僢僋(PAL傪帺摦敾暿)
//
void	ROM::FilenameCheck( const char* fname )
{
	unsigned char*	p = (unsigned char*)fname;

	while( *p != (unsigned char)'\0' ) {
		if( *p == (unsigned char)'(' ) {
			if( _mbsnbicmp( p, (unsigned char*)"(E)", 3 ) == 0 ) {
				bPAL = TRUE;
				return;
			}
		}

		p = _mbsinc(p);
	}
}

