/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */


#include "PrecompiledHeader.h"
#include "Common.h"

#include "Gif.h"
#include "GS.h"
#include "Vif.h"
#include "IPU/IPU.h"

#include "ps2/NewDmac.h"

using namespace EE_DMAC;

__aligned16 PeripheralFifoPack g_fifo;


template< uint size >
void FifoRingBuffer<size>::SendToPeripheral(Fnptr_ToPeripheral toFunc, const char* name)
{
	uint transferred = toFunc( buffer, size, readpos, qwc );
	qwc -= transferred;

	readpos += transferred;
	readpos &= size-1;
}

template< uint size >
void FifoRingBuffer<size>::HwWrite(Fnptr_ToPeripheral toFunc, const u128* src, const char* name)
{
	GIF_LOG("WriteFIFO/%s <- %ls (FQC=%u)", src->ToString().c_str(), qwc);

	if (qwc >= size) return;
	WriteQWC(src);

	if (qwc >= size)
	{
		ProcessFifoEvent();
		CPU_ClearEvent(FIFO_EVENT);
	}
	else
	{
		CPU_ScheduleEvent(FIFO_EVENT, 128);
	}
}

template< uint size >
void FifoRingBuffer<size>::ReadQWC(u128* dest)
{
	pxAssume(qwc);
	CopyQWC(dest, &buffer[readpos]);
	readpos = (readpos+1) & (size-1);
	--qwc;
}

template< uint size >
void FifoRingBuffer<size>::WriteQWC(const u128* src)
{
	pxAssume(qwc < size);
	CopyQWC(&buffer[writepos], src);
	writepos = (writepos+1) & (size-1);
	++qwc;
}

// Notes on FIFO implementation
//
// The FIFO consists of four separate pages of HW register memory, each mapped to a
// PS2 device.  They are listed as follows:
//
// 0x4000 - 0x5000 : VIF0  (all registers map to 0x4000)
// 0x5000 - 0x6000 : VIF1  (all registers map to 0x5000)
// 0x6000 - 0x7000 : GS    (all registers map to 0x6000)
// 0x7000 - 0x8000 : IPU   (registers map to 0x7000 and 0x7010, respectively)

void __fastcall ReadFIFO_VIF1(mem128_t* out)
{
	// Notes:
	//  * VIF1 is only readable if the FIFO direction has been reversed.
	//  * FDR should only be reversible if the VIF is "clear" (idle), so there shouldn't
	//    be any stall conditions or other obstructions  (writes to FDR other times are
	//    likely disregarded or yield indeterminate results).

	//if (vif1Regs.stat.test(VIF1_STAT_INT | VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS) )
	//	DevCon.Warning( "Reading from vif1 fifo when stalled" );

	// [Ps2Confirm] We're not sure what real hardware does when the VIF1 FIFO is empty or
	// when the direction is pointing the wrong way.  It could either deadlock the EE, return 0,
	// or return some indeterminate value (equivalent to leaving the out unmodified, etc.).
	// Current assumption is indeterminate.

	if (!vif1Regs.stat.FDR)
	{
		VIF_LOG("Read from VIF1 FIFO ignored due to FDR=0.");
		return;
	}

	if (g_fifo.vif1.IsEmpty())
	{
		// FIFO is empty, so queue up 16 qwc from the GS plugin.  Operating in bursts should
		// keep framerates from being too terribly miserable.

		// Note: The only safe way to download data form the GS is to perform a full sync/flush
		// of the GS ringbuffer.  It's slow but absolutely unavoidable.  Fortunately few games
		// rely on this feature (its quite slow on the PS2 as well for the same reason).

		GetMTGS().WaitGS();

		g_fifo.vif1.qwc = GSreadFIFO2((u64*)g_fifo.vif1.buffer, FifoSize_Vif1);
		g_fifo.vif1.readpos = 0;
		g_fifo.vif1.writepos = g_fifo.vif1.qwc;

		if(!pxAssertDev(!g_fifo.vif1.IsEmpty(), "Reading from an empty VIF1 FIFO (FQC=0)")) return;
	}

	g_fifo.vif1.ReadQWC(out);
	VIF_LOG("ReadFIFO/VIF1 -> %ls", out->ToString().c_str());
}

void __fastcall ReadFIFO_IPUout(mem128_t* out)
{
	// Games should always check the fifo before reading from it -- so if the FIFO has no data
	// its either some glitchy game or a bug in pcsx2.

	if (!pxAssertDev( g_fifo.ipu0.qwc > 0, "Attempted read from IPUout's FIFO, but the FIFO is empty!" )) return;
	g_fifo.ipu0.ReadQWC(out);
}

// --------------------------------------------------------------------------------------
//  WriteFIFO Strategies
// --------------------------------------------------------------------------------------
//  * Writes to VIF FIFOs should only occur while the DMA is inactive, since writing the FIFO
//    directly while DMA is running likely yields indeterminate results (corrupted VIFcodes
//    most likely). [assertion checked]
//
// The VIF/GIF could be stalled, in which case the data will be denied; so we *must* emulate
// the fifo so that the EE has a place to queue commands until it deals with the stall
// condition. The FIFO will be automatically drained whenever the VIF STAT register is written
// (which is the only action that can alleviate stalls).
//
// Technically each FIFO should start processing almost immediately (2-4 cycle delay from the
// perspective of the EE).  But since the whole point of the FIFO is to eliminate the need
// for the EE to ever care about such prompt responsiveness, we delay longer.  This gives the
// EE some time to queue up a few commands before we try to process them.
//
// (we could process immediately anyway, but I prefer to keep the hwRegister handlers as
//  simple as possible and defer complicated VIFcode processing until the ee event test-- air).
//
// [Ps2Confirm] If the FIFO is full, we don't know what to do (yet) so we just disregard the
// write for now.
//
void __fastcall WriteFIFO_VIF0(const mem128_t* value)
{
	pxAssume( !vif0dma.chcr.STR );
	g_fifo.gif.HwWrite(EE_DMAC::toVIF0, value, "VIF0");
}

void __fastcall WriteFIFO_VIF1(const mem128_t* value)
{
	pxAssume( !vif1dma.chcr.STR );

	// Writes to VIF1 FIFO when its in Source mode (VIF->memory transfer direction) are likely disregarded.
	if (vif1Regs.stat.FDR) return;

	g_fifo.gif.HwWrite(EE_DMAC::toVIF1, value, "VIF1");
}

void __fastcall WriteFIFO_GIF(const mem128_t* value)
{
	// GIF FIFO uses PATH3 for transfer (same as GIF DMA).

	pxAssume( !gifdma.chcr.STR );
	g_fifo.gif.HwWrite(EE_DMAC::toGIF, value, "GIF");
}

void __fastcall WriteFIFO_IPUin(const mem128_t* value)
{
	pxAssume( !gifdma.chcr.STR );
	g_fifo.ipu1.HwWrite(EE_DMAC::toIPU, value, "toIPU");
}

void ProcessFifoEvent()
{
	CPU_ClearEvent(FIFO_EVENT);
	if (g_fifo.vif0.IsEmpty())
	{
		g_fifo.vif0.SendToPeripheral(EE_DMAC::toVIF0, "VIF0");
	}

	if (g_fifo.vif1.IsEmpty() && !vif1Regs.stat.FDR)
	{
		// cannot drain the VIF1 FIFO when its in Source (toMemory) mode (FDR=1).  The EE has
		// to read from it or reset it manually.

		g_fifo.vif1.SendToPeripheral(EE_DMAC::toVIF1, "VIF1");
	}

	if (g_fifo.gif.IsEmpty())
	{
		// GIF FIFO uses PATH3 for transfer (same as GIF DMA).
		// So if PATH3 cannot get arbitration rights to the GIF, then the FIFO cannot empty itself.

		g_fifo.gif.SendToPeripheral(EE_DMAC::toGIF, "GIF");
	}

	// [TODO] : IPU FIFOs (but those need some work)
}
