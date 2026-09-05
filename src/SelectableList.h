#pragma once

#include <cstdint>
#include <list>

template<typename T>
class SelectableList
{
public:
    typedef typename std::list<T>::iterator Iterator;

    SelectableList()
        : m_HasSelection(false)
    {
    }

    T *GetSelected()
    {
        if (!m_HasSelection)
            return nullptr;

        return &(*m_SelectedIter);
    }

    void SetSelected(Iterator it)
    {
        m_SelectedIter = it;
        m_HasSelection = (it != m_Items.end());
    }

    bool SelectPrevious()
    {
        if (!m_HasSelection || m_SelectedIter == m_Items.begin())
            return false;

        --m_SelectedIter;

        return true;
    }

    bool SelectNext()
    {
        if (!m_HasSelection)
            return false;

        Iterator next = m_SelectedIter;
        ++next;

        if (next == m_Items.end())
            return false;

        m_SelectedIter = next;
        return true;
    }

    Iterator PushBack(const T &item)
    {
        m_Items.push_back(item);
        Iterator it = m_Items.end();
        --it;

        if (!m_HasSelection)
        {
            m_SelectedIter = it;
            m_HasSelection = true;
        }

        return it;
    }

    void Remove(Iterator it)
    {
        bool wasSelected = m_HasSelection && (it == m_SelectedIter);

        if (wasSelected)
        {
            Iterator next = it;
            ++next;

            if (next != m_Items.end())
            {
                m_SelectedIter = next;
            }
            else if (it != m_Items.begin())
            {
                Iterator prev = it;
                --prev;
                m_SelectedIter = prev;
            }
            else
            {
                m_HasSelection = false;
            }
        }

        m_Items.erase(it);
    }

    template<typename Predicate>
    bool RemoveIf(Predicate predicate)
    {
        Iterator toRemove = m_Items.end();
        for (Iterator it = m_Items.begin(); it != m_Items.end(); ++it)
        {
            if (predicate(*it))
            {
                toRemove = it;
                break;
            }
        }

        if (toRemove == m_Items.end())
            return false;

        Remove(toRemove);

        return true;
    }

private:
    std::list<T> m_Items;
    Iterator m_SelectedIter;
    bool m_HasSelection;
};
