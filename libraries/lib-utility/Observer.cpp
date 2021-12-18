/**********************************************************************

  Audacity: A Digital Audio Editor

  @file Observer.cpp
  @brief Publisher-subscriber pattern, also known as Observer

  Paul Licameli

**********************************************************************/
#include "Observer.h"

namespace Observer {

Message::~Message() = default;

namespace detail {

RecordList::RecordList( ExceptionPolicy *pPolicy, Visitor visitor )
   : m_pPolicy{ pPolicy }
   , m_visitor{ move(visitor) }
{
   assert(m_visitor); // precondition
}

RecordList::~RecordList() noexcept
{
   //! Non-defaulted destructor.  Beware stack growth
   auto pRecord = next;
   while (pRecord)
      pRecord = move(pRecord->next);
}

void RecordList::PushFront(std::shared_ptr<RecordBase> &&pRecord)
{
   assert(pRecord); // precondition
   if (auto &pNext = (pRecord->next = move(next)))
      pNext->prev = pRecord;
   pRecord->prev = weak_from_this();
   next = move(pRecord);
}

void RecordList::Remove(std::shared_ptr<RecordBase> pRecord) noexcept
{
   auto wPrev = move(pRecord->prev);
   auto pPrev = wPrev.lock();
   assert(pPrev); // See constructor and PushFront
   // Do not move from pRecord->next, see Visit
   if (auto &pNext = (pPrev->next = pRecord->next))
      pNext->prev = move(wPrev);
}

bool RecordList::Visit(const void *arg)
{
   assert(m_visitor); // See constructor
   if (m_pPolicy)
      m_pPolicy->OnBeginPublish();
   for (auto pRecord = next; pRecord; pRecord = pRecord->next) {
      // Calling foreign code!  Which is why we have an exception policy.
      // The generated visitor handles per-callback exception policy.
      if (m_visitor(*pRecord, arg))
         return true;
      // pRecord might have been removed from the list by the callback,
      // but pRecord->next is unchanged.  We won't see callbacks added by
      // the callback, because they are earlier in the list.
   }
   // Intentionally not in a finally():
   if (m_pPolicy)
      m_pPolicy->OnEndPublish();
   return false;
}

}

ExceptionPolicy::~ExceptionPolicy() noexcept = default;

Subscription::Subscription() = default;
Subscription::Subscription(std::weak_ptr<detail::RecordBase> pRecord)
   : m_wRecord{ move(pRecord) } {}
Subscription::Subscription(Subscription &&) = default;
Subscription &Subscription::operator=(Subscription &&other)
{
   const bool inequivalent =
      m_wRecord.owner_before(other.m_wRecord) ||
      other.m_wRecord.owner_before(m_wRecord);
   if (inequivalent) {
      Reset();
      m_wRecord = move(other.m_wRecord);
   }
   return *this;
}

void Subscription::Reset() noexcept
{
   if (auto pRecord = m_wRecord.lock())
      pRecord->list.Remove(move(pRecord));
   m_wRecord.reset();
}

}
