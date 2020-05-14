/**********************************************************************

   Audacity - A Digital Audio Editor
   Copyright 1999-2009 Audacity Team
   File License: wxWidgets

   Dan Horgan

******************************************************************//**

\file ResponseQueue.cpp
\brief Contains definitions for the ResponseQueue class

*//*******************************************************************/

#include "ResponseQueue.h"

#include <queue>
#include <string>

ResponseQueue::ResponseQueue()
{ }

ResponseQueue::~ResponseQueue()
{ }

void ResponseQueue::AddResponse(Response response)
{
   std::lock_guard< std::mutex > locker{ mMutex };
   mResponses.push(response);
   mCondition.notify_one();
}

Response ResponseQueue::WaitAndGetResponse()
{
   std::unique_lock< std::mutex > lock{ mMutex };
   while (mResponses.empty())
      mCondition.wait( lock );
   wxASSERT(!mResponses.empty());
   Response msg = mResponses.front();
   mResponses.pop();
   return msg;
}
