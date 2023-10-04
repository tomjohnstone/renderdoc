/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2020-2023 Baldur Karlsson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include "d3d12_command_list.h"

template <typename SerialiserType>
bool WrappedID3D12GraphicsCommandList::Serialise_DispatchMesh(SerialiserType &ser,
                                                              UINT ThreadGroupCountX,
                                                              UINT ThreadGroupCountY,
                                                              UINT ThreadGroupCountZ)
{
  ID3D12GraphicsCommandList6 *pCommandList = this;
  SERIALISE_ELEMENT(pCommandList);
  SERIALISE_ELEMENT(ThreadGroupCountX).Important();
  SERIALISE_ELEMENT(ThreadGroupCountY).Important();
  SERIALISE_ELEMENT(ThreadGroupCountZ).Important();

  SERIALISE_CHECK_READ_ERRORS();

  if(IsReplayingAndReading())
  {
    m_Cmd->m_LastCmdListID = GetResourceManager()->GetOriginalID(GetResID(pCommandList));

    if(IsActiveReplaying(m_State))
    {
      if(m_Cmd->InRerecordRange(m_Cmd->m_LastCmdListID))
      {
        ID3D12GraphicsCommandListX *list = m_Cmd->RerecordCmdList(m_Cmd->m_LastCmdListID);

        uint32_t eventId = m_Cmd->HandlePreCallback(list, ActionFlags::Dispatch);
        Unwrap(list)->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
        if(eventId && m_Cmd->m_ActionCallback->PostDispatch(eventId, list))
        {
          Unwrap6(list)->DispatchMesh(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
          m_Cmd->m_ActionCallback->PostRedispatch(eventId, list);
        }
      }
    }
    else
    {
      Unwrap6(pCommandList)->DispatchMesh(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);

      m_Cmd->AddEvent();

      ActionDescription action;
      action.dispatchDimension[0] = ThreadGroupCountX;
      action.dispatchDimension[1] = ThreadGroupCountY;
      action.dispatchDimension[2] = ThreadGroupCountZ;

      action.flags |= ActionFlags::Dispatch;

      m_Cmd->AddAction(action);
    }
  }

  return true;
}

void STDMETHODCALLTYPE WrappedID3D12GraphicsCommandList::DispatchMesh(UINT ThreadGroupCountX,
                                                                      UINT ThreadGroupCountY,
                                                                      UINT ThreadGroupCountZ)
{
  SERIALISE_TIME_CALL(m_pList6->DispatchMesh(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ));

  if(IsCaptureMode(m_State))
  {
    CACHE_THREAD_SERIALISER();
    ser.SetActionChunk();
    SCOPED_SERIALISE_CHUNK(D3D12Chunk::List_DispatchMesh);
    Serialise_DispatchMesh(ser, ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);

    m_ListRecord->AddChunk(scope.Get(m_ListRecord->cmdInfo->alloc));
  }
}

INSTANTIATE_FUNCTION_SERIALISED(void, WrappedID3D12GraphicsCommandList, DispatchMesh,
                                UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ);
