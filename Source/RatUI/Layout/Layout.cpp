#include <RatUI/Layout/Layout.h>

namespace RatUI
{
    void LayoutNode::MarkDirty()
    {
        if ( !Layout.IsDirty )
        {
            Layout.IsDirty = true;
            if ( m_Parent ) m_Parent->MarkDescendantDirty();
        }
    }
    
    void LayoutNode::MarkDescendantDirty()
    {
        if ( Layout.IsDescendantDirty ) return; // Already marked as having a dirty descendant, so we can stop here.

        Layout.IsDescendantDirty = true;

        LayoutNode* parent = m_Parent;
        LayoutNode* current = this;

        while ( parent )
        {
            if ( !parent->Layout.IsDescendantDirty )
            {
                parent->Layout.IsDescendantDirty = true;
                current = parent;
                parent = parent->m_Parent;
            }
            else
            {
                break; // Ancestors are already marked as having a dirty descendant, so we can stop here.
            }
        }
    }

    void LayoutNode::DetachFromParent()
    {
        if ( !m_Parent )
            return;

        if ( m_PrevSibling )
            m_PrevSibling->m_NextSibling = m_NextSibling;
        else
            m_Parent->m_FirstChild = m_NextSibling;

        if ( m_NextSibling )
            m_NextSibling->m_PrevSibling = m_PrevSibling;
        else
            m_Parent->m_LastChild = m_PrevSibling;

        m_Parent->m_ChildCount--;
        m_Parent = nullptr;
        m_PrevSibling = nullptr;
        m_NextSibling = nullptr;
    }

    void LayoutNode::PushBackChild( LayoutNode& a_Child )
    {
        a_Child.DetachFromParent();

        a_Child.m_Parent = this;
        a_Child.m_NextSibling = nullptr;

        if ( m_LastChild )
        {
            m_LastChild->m_NextSibling = &a_Child;
            a_Child.m_PrevSibling = m_LastChild;
        }
        else
        {
            m_FirstChild = &a_Child;
            a_Child.m_PrevSibling = nullptr;
        }

        m_LastChild = &a_Child;
        ++m_ChildCount;
    }

    void LayoutNode::PushFrontChild( LayoutNode& a_Child )
    {
        a_Child.DetachFromParent();

        a_Child.m_Parent = this;
        a_Child.m_PrevSibling = nullptr;

        if ( m_FirstChild )
        {
            m_FirstChild->m_PrevSibling = &a_Child;
            a_Child.m_NextSibling = m_FirstChild;
        }
        else
        {
            m_LastChild = &a_Child;
            a_Child.m_NextSibling = nullptr;
        }

        m_FirstChild = &a_Child;
        ++m_ChildCount;
    }

    void LayoutNode::InsertChildAfter( LayoutNode& a_Child, LayoutNode& a_Sibling )
    {
        if ( !a_Sibling.m_Parent || a_Sibling.m_Parent != this )
        {
            RATUI_USER_ASSERT( false, "Sibling node is not a child of this parent" );
            return;
        }

        a_Child.DetachFromParent();

        a_Child.m_Parent = this;
        a_Child.m_PrevSibling = &a_Sibling;
        a_Child.m_NextSibling = a_Sibling.m_NextSibling;

        if ( a_Sibling.m_NextSibling )
            a_Sibling.m_NextSibling->m_PrevSibling = &a_Child;
        else
            m_LastChild = &a_Child;

        a_Sibling.m_NextSibling = &a_Child;
        ++m_ChildCount;
    }

    void LayoutNode::InsertChildBefore( LayoutNode& a_Child, LayoutNode& a_Sibling )
    {
        if ( !a_Sibling.m_Parent || a_Sibling.m_Parent != this )
        {
            RATUI_USER_ASSERT( false, "Sibling node is not a child of this parent" );
            return;
        }

        a_Child.DetachFromParent();

        a_Child.m_Parent = this;
        a_Child.m_NextSibling = &a_Sibling;
        a_Child.m_PrevSibling = a_Sibling.m_PrevSibling;

        if ( a_Sibling.m_PrevSibling )
            a_Sibling.m_PrevSibling->m_NextSibling = &a_Child;
        else
            m_FirstChild = &a_Child;

        a_Sibling.m_PrevSibling = &a_Child;
        ++m_ChildCount;
    }

} // namespace RatUI
