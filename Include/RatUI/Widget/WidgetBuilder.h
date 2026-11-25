#pragma once
#include "Scene.h"
#include "ButtonWidget.h"
#include "InputTextWidget.h"
#include "PanelWidget.h"
#include "ScrollContainerWidget.h"
#include "SliderWidget.h"
#include "TextWidget.h"

/**
 * @file WidgetBuilder.h
 * @brief Declarative building of node and widget trees.
 *
 * @code
 * SceneBuilder ui{ scene };
 * ui.Root( ui.VBox().Padding( 16_u )(
 *     ui.Text( "Title" ),
 *     ui.HBox()( ui.Button( "OK", onOk ), ui.Button( "Cancel", onCancel ) )
 * ) );
 * @endcode
 *
 * - VBox, HBox, Grid and Node are bare layout nodes: no drawing, input or focus. Use Panel() for a drawn box.
 * - Only containers (nodes, and widgets with c_HasChildren) take children and child-layout setters.
 * - Anything never attached or made the root stays in the scene but is not laid out or drawn.
 *
 * Custom widgets: declare `using Super = Base;` to inherit the base's setters, specialize WidgetBuilderSetters to add
 * your own, and add factories with a mixin: `using GameSceneBuilder = BasicSceneBuilder<StandardFactories, GameFactories>;`.
 *
 * @code
 * class GaugeWidget : public PanelWidget { public: using Super = PanelWidget; f32 Fill = 0.f; };
 *
 * template<class E>
 * struct RatUI::WidgetBuilderSetters<GaugeWidget, E> : RatUI::WidgetBuilderSetters<GaugeWidget::Super, E>
 * {
 *     E Fill( f32 a_Fill ) const { const E& self = static_cast<const E&>( *this ); self->Fill = a_Fill; return self; }
 * };
 *
 * template<class B>
 * struct GameFactories
 * {
 *     WidgetBuilder<GaugeWidget> Gauge( f32 a_Fill ) const { return MakeWidget<GaugeWidget>( static_cast<const B&>( *this ) ).Fill( a_Fill ); }
 * };
 * @endcode
 */
namespace RatUI
{
    namespace Detail
    {
        template<typename Self, typename Mixin>
        RATUI_NODISCARD const Self& SelfOf( const Mixin* a_Mixin ) { return static_cast<const Self&>( *a_Mixin ); }

        struct NoSetters {};
    }

    /** @brief A NodeBuilder or WidgetBuilder. */
    template<typename T>
    concept LayoutBuilder = requires( const T& a_Builder )
    {
        { a_Builder.GetLayout() } -> std::same_as<LayoutNode&>;
    };

    // ========================================================================
    // Layout setters (shared by NodeBuilder and WidgetBuilder)
    // ========================================================================

    /** @brief A builder that can take children. */
    template<typename T>
    concept ContainerBuilder = T::c_HasChildren;

    /**
     * @brief Layout setters shared by every builder.
     * Base keeps everything in one inheritance chain, so builders stay pointer-sized on MSVC.
     */
    template<typename Self, typename Base = Detail::NoSetters>
    class LayoutSetters : public Base
    {
    public:
        /** @brief Appends children in order. Can be called repeatedly. */
        template<LayoutBuilder... Children>
            requires ContainerBuilder<Self>
        Self operator()( const Children&... a_Children ) const
        {
            LayoutNode& slot = This().GetChildSlot();
            ( Attach( slot, a_Children.GetLayout() ), ... );
            return This();
        }

        // - Child layout (applied to the child slot, e.g. a ScrollBox's content)

        Self LayoutType( ELayoutType a_Type ) const requires ContainerBuilder<Self> { Slot().LayoutType( a_Type ); return This(); }
        Self Spacing( Unit a_Spacing ) const requires ContainerBuilder<Self>        { Slot().Spacing( a_Spacing ); return This(); }
        Self ChildAlign( EAlign a_Align ) const requires ContainerBuilder<Self>     { Slot().ChildAlign( a_Align ); return This(); }
        Self GridColumns( u16 a_Columns ) const requires ContainerBuilder<Self>     { Slot().GridColumns( a_Columns ); return This(); }
        Self GridRows( u16 a_Rows ) const requires ContainerBuilder<Self>           { Slot().GridRows( a_Rows ); return This(); }

        Self Vertical() const requires ContainerBuilder<Self>   { return LayoutType( ELayoutType::Vertical ); }
        Self Horizontal() const requires ContainerBuilder<Self> { return LayoutType( ELayoutType::Horizontal ); }
        Self Overlay() const requires ContainerBuilder<Self>    { return LayoutType( ELayoutType::Overlay ); }
        Self Grid( u16 a_Columns ) const requires ContainerBuilder<Self> { return LayoutType( ELayoutType::Grid ).GridColumns( a_Columns ).GridRows( 0 ); }

        // - Own layout

        Self SelfAlign( EAlign a_Align ) const            { Layout().SelfAlign( a_Align ); return This(); }
        Self Visibility( EVisibility a_Visibility ) const { Layout().Visibility( a_Visibility ); return This(); }
        Self FocusScope( bool a_IsFocusScope = true ) const { Layout().FocusScope( a_IsFocusScope ); return This(); }

        Self Padding( Edges a_Padding ) const { Layout().Padding( a_Padding ); return This(); }
        Self Padding( Unit a_Padding ) const  { Layout().Padding( Edges::All( a_Padding ) ); return This(); }
        Self Margin( Edges a_Margin ) const   { Layout().Margin( a_Margin ); return This(); }
        Self Margin( Unit a_Margin ) const    { Layout().Margin( Edges::All( a_Margin ) ); return This(); }

        Self PositionMode( EPositioning a_Mode ) const { Layout().PositionMode( a_Mode ); return This(); }
        Self Anchor( RatUI::Anchor a_Anchor ) const    { Layout().Anchor( a_Anchor ); return This(); }
        Self SafeAreaEdges( ESafeAreaEdges a_Edges ) const { Layout().SafeAreaEdges( a_Edges ); return This(); }

        Self WidthMode( ESizing a_Mode ) const  { Layout().WidthMode( a_Mode ); return This(); }
        Self HeightMode( ESizing a_Mode ) const { Layout().HeightMode( a_Mode ); return This(); }

        Self FixedWidth( Unit a_Width ) const               { Layout().FixedWidth( a_Width ); return This(); }
        Self FixedHeight( Unit a_Height ) const             { Layout().FixedHeight( a_Height ); return This(); }
        Self FixedSize( Unit a_Width, Unit a_Height ) const { Layout().FixedWidth( a_Width ).FixedHeight( a_Height ); return This(); }
        Self PercentWidth( f32 a_Percent ) const            { Layout().PercentWidth( a_Percent ); return This(); }
        Self PercentHeight( f32 a_Percent ) const           { Layout().PercentHeight( a_Percent ); return This(); }

        Self FlexWidth() const  { Layout().FlexWidth(); return This(); }
        Self FlexHeight() const { Layout().FlexHeight(); return This(); }
        Self Flex() const       { Layout().FlexWidth().FlexHeight(); return This(); }
        Self Grow( f32 a_FlexGrow ) const { Layout().FlexGrow( a_FlexGrow ); return This(); }

        Self ContentWidth() const  { Layout().WidthMode( ESizing::Content ); return This(); }
        Self ContentHeight() const { Layout().HeightMode( ESizing::Content ); return This(); }

        Self SizeConstraints( Constraints a_Constraints ) const { Layout().SizeConstraints( a_Constraints ); return This(); }
        Self MinWidth( Unit a_Width ) const   { return EditConstraints( [&]( Constraints& c ) { c.Min[0] = a_Width; } ); }
        Self MinHeight( Unit a_Height ) const { return EditConstraints( [&]( Constraints& c ) { c.Min[1] = a_Height; } ); }
        Self MaxWidth( Unit a_Width ) const   { return EditConstraints( [&]( Constraints& c ) { c.Max[0] = a_Width; } ); }
        Self MaxHeight( Unit a_Height ) const { return EditConstraints( [&]( Constraints& c ) { c.Max[1] = a_Height; } ); }

    private:
        const Self& This() const { return Detail::SelfOf<Self>( this ); }
        LayoutNode& Layout() const { return This().GetLayout(); }
        LayoutNode& Slot() const   { return This().GetChildSlot(); }

        static void Attach( LayoutNode& a_Slot, LayoutNode& a_Child )
        {
            a_Slot.PushBackChild( a_Child );
            a_Child.MarkDirty();
        }

        template<typename Func>
        Self EditConstraints( Func&& a_Func ) const
        {
            Constraints constraints = Layout().Style.SizeConstraints;
            a_Func( constraints );
            Layout().SizeConstraints( constraints );
            return This();
        }
    };

    // ========================================================================
    // NodeBuilder
    // ========================================================================

    /** @brief Builds a bare layout node. */
    class NodeBuilder : public LayoutSetters<NodeBuilder>
    {
    public:
        static constexpr bool c_HasChildren = true;

        NodeBuilder( Scene& a_Scene, NodeID a_ID ) : m_Scene( &a_Scene ), m_ID( a_ID ) {}

        RATUI_NODISCARD NodeID GetID() const     { return m_ID; }
        RATUI_NODISCARD Scene& GetScene() const  { return *m_Scene; }

        RATUI_NODISCARD LayoutNode& GetLayout() const
        {
            LayoutNode* node = m_Scene->GetLayoutNode( m_ID );
            RATUI_USER_ASSERT( node, "NodeBuilder refers to a node that no longer exists." );
            return *node;
        }

        RATUI_NODISCARD LayoutNode& GetChildSlot() const { return GetLayout(); }

        NodeBuilder Bind( NodeID& a_Out ) const { a_Out = m_ID; return *this; }

        /** @brief Escape hatch for anything without a setter. */
        template<std::invocable<LayoutNode&> Func>
        NodeBuilder With( Func&& a_Func ) const { std::forward<Func>( a_Func )( GetLayout() ); return *this; }

    private:
        Scene* m_Scene;
        NodeID m_ID;
    };

    // ========================================================================
    // Widget setters
    // ========================================================================

    namespace Detail
    {
        template<typename W>
        concept HasSuperWidget = requires { typename W::Super; }
                              && std::derived_from<W, typename W::Super>
                              && !std::same_as<W, typename W::Super>;
    }

    /**
     * @brief Widget-specific setters for WidgetBuilder<W> (E). Specialize for your widgets; hold no data.
     * Unspecialized, W gets W::Super's setters. Declare Super in every widget, since it is inherited.
     */
    template<typename W, typename E>
    struct WidgetBuilderSetters {};

    template<typename W, typename E>
        requires Detail::HasSuperWidget<W>
    struct WidgetBuilderSetters<W, E> : WidgetBuilderSetters<typename W::Super, E> {};

    /** @brief Setters for widgets with a TextLayoutStyle. */
    template<typename E>
    struct TextStyleSetters
    {
        E Style( const TextLayoutStyle& a_Style ) const { const E& self = Detail::SelfOf<E>( this ); self->SetLayoutStyle( a_Style ); return self; }
        E Font( FontHandle a_Font ) const { return EditTextStyle( [&]( TextLayoutStyle& s ) { s.Font = a_Font; } ); }
        E Size( Unit a_Size ) const       { return EditTextStyle( [&]( TextLayoutStyle& s ) { s.Size = a_Size; } ); }

    protected:
        template<typename Func>
        E EditTextStyle( Func&& a_Func ) const
        {
            const E& self = Detail::SelfOf<E>( this );
            TextLayoutStyle style = self->GetLayoutStyle();
            a_Func( style );
            self->SetLayoutStyle( style );
            return self;
        }
    };

    // - Built-in widgets

    template<typename E>
    struct WidgetBuilderSetters<ButtonBaseWidget, E>
    {
        E OnClick( ButtonBaseWidget::OnClickCallback a_OnClick ) const
        {
            const E& self = Detail::SelfOf<E>( this );
            self->OnClick = std::move( a_OnClick );
            return self;
        }
    };

    template<typename E>
    struct WidgetBuilderSetters<TextWidget, E> : TextStyleSetters<E>
    {
        E Wrap( TextWrap a_Wrap ) const              { return this->EditTextStyle( [&]( TextLayoutStyle& s ) { s.Wrap = a_Wrap; } ); }
        E NoWrap() const                             { return Wrap( TextWrap::NoWrap() ); }
        E Overflow( ETextOverflow a_Overflow ) const { return this->EditTextStyle( [&]( TextLayoutStyle& s ) { s.Overflow = a_Overflow; } ); }
        E MaxLines( u32 a_MaxLines ) const           { return this->EditTextStyle( [&]( TextLayoutStyle& s ) { s.MaxLines = a_MaxLines; } ); }
    };

    template<typename E>
    struct WidgetBuilderSetters<InputTextWidget, E> : TextStyleSetters<E>
    {
        E Text( StringView a_Text ) const        { const E& self = Detail::SelfOf<E>( this ); self->SetText( a_Text ); return self; }
        E Placeholder( StringView a_Text ) const { const E& self = Detail::SelfOf<E>( this ); self->SetPlaceholder( a_Text ); return self; }

        E OnChanged( Callback<const String&> a_OnChanged ) const
        {
            const E& self = Detail::SelfOf<E>( this );
            self->OnChanged = std::move( a_OnChanged );
            return self;
        }

        E OnSubmit( Callback<const String&> a_OnSubmit ) const
        {
            const E& self = Detail::SelfOf<E>( this );
            self->OnSubmit = std::move( a_OnSubmit );
            return self;
        }
    };

    template<typename E>
    struct WidgetBuilderSetters<SliderWidget, E>
    {
        E Orientation( EOrient a_Orientation ) const { const E& self = Detail::SelfOf<E>( this ); self->Orientation = a_Orientation; return self; }
        E Step( f32 a_Step ) const                   { const E& self = Detail::SelfOf<E>( this ); self->Step = a_Step; return self; }

        /** @brief Also called immediately with the current value. */
        template<std::invocable<const f32&> Func>
        E OnValueChanged( Func&& a_Func ) const
        {
            const E& self = Detail::SelfOf<E>( this );
            self->Value.Subscribe( std::forward<Func>( a_Func ) );
            return self;
        }
    };

    // ========================================================================
    // WidgetBuilder
    // ========================================================================

    /** @brief Builds a widget. A non-owning pointer; setters apply immediately. */
    template<std::derived_from<IWidget> W>
    class WidgetBuilder : public LayoutSetters<WidgetBuilder<W>, WidgetBuilderSetters<W, WidgetBuilder<W>>>
    {
    public:
        using WidgetType = W;

        static constexpr bool c_HasChildren = W::c_HasChildren;

        explicit WidgetBuilder( W& a_Widget ) : m_Widget( &a_Widget ) {}

        template<std::derived_from<W> Derived>
        WidgetBuilder( WidgetBuilder<Derived> a_Other ) : m_Widget( a_Other.Get() ) {}

        RATUI_NODISCARD W* Get()        const { return m_Widget; }
        RATUI_NODISCARD W* operator->() const { return m_Widget; }
        RATUI_NODISCARD W& operator*()  const { return *m_Widget; }
        operator W*()                   const { return m_Widget; }

        RATUI_NODISCARD LayoutNode& GetLayout() const { return m_Widget->GetLayout(); }

        RATUI_NODISCARD LayoutNode& GetChildSlot() const
        {
            LayoutNode* slot = m_Widget->GetScene().GetLayoutNode( m_Widget->GetChildSlotID() );
            RATUI_USER_ASSERT( slot, "Cannot add children: the widget has no valid child slot." );
            return *slot;
        }

        WidgetBuilder Bind( W*& a_Out ) const { a_Out = m_Widget; return *this; }

        /** @brief Escape hatch for anything without a setter. */
        template<std::invocable<W&> Func>
        WidgetBuilder With( Func&& a_Func ) const { std::forward<Func>( a_Func )( *m_Widget ); return *this; }

        WidgetBuilder Theme( ThemeHandle a_Theme ) const requires ( W::template HasMixin<ThemeMixin> )
        {
            m_Widget->Theme = std::move( a_Theme );
            return *this;
        }

        WidgetBuilder DebugName( String a_Name ) const requires ( W::template HasMixin<DebugMixin> )
        {
            m_Widget->DebugName = std::move( a_Name );
            return *this;
        }

    private:
        W* m_Widget;
    };

    // ========================================================================
    // SceneBuilder
    // ========================================================================

    /** @brief Creates nodes and widgets in a scene. Named factories come from the Factories mixins. */
    template<template<typename> typename... Factories>
    class BasicSceneBuilder : public Factories<BasicSceneBuilder<Factories...>>...
    {
    public:
        explicit BasicSceneBuilder( Scene& a_Scene ) : m_Scene( a_Scene ) {}

        RATUI_NODISCARD Scene& GetScene() const { return m_Scene; }

        template<std::derived_from<IWidget> W, typename... Args>
        WidgetBuilder<W> MakeWidget( Args&&... a_Args ) const
        {
            return WidgetBuilder<W>{ *m_Scene.CreateWidget<W>( c_InvalidNodeID, std::forward<Args>( a_Args )... ) };
        }

        NodeBuilder MakeNode() const
        {
            return NodeBuilder{ m_Scene, m_Scene.CreateLayoutNode().ID };
        }

        /** @brief Sets the scene root. It must have no parent; the old root is not destroyed. */
        template<LayoutBuilder T>
        T Root( T a_Builder ) const
        {
            LayoutNode& node = a_Builder.GetLayout();
            RATUI_USER_ASSERT( !node.Parent(), "Root(): the node is already a child of another node." );
            m_Scene.RootWidget = node.ID;
            return a_Builder;
        }

    private:
        Scene& m_Scene;
    };

    /** @brief For factory mixins: saves writing ui.template MakeWidget<W>(). */
    template<std::derived_from<IWidget> W, typename BuilderType, typename... Args>
    WidgetBuilder<W> MakeWidget( const BuilderType& a_Builder, Args&&... a_Args )
    {
        return a_Builder.template MakeWidget<W>( std::forward<Args>( a_Args )... );
    }

    template<typename BuilderType>
    NodeBuilder MakeNode( const BuilderType& a_Builder )
    {
        return a_Builder.MakeNode();
    }

    template<typename B>
    struct StandardFactories
    {
        // - Layout nodes
        NodeBuilder Node() const    { return MakeNode( Ui() ); }
        NodeBuilder VBox() const    { return Node().Vertical(); }
        NodeBuilder HBox() const    { return Node().Horizontal(); }
        NodeBuilder Overlay() const { return Node(); }

        NodeBuilder Grid( u16 a_Columns ) const { return Node().Grid( a_Columns ); }

        // - Containers

        /** @brief A drawn box (fill, border, theme). */
        WidgetBuilder<PanelWidget> Panel() const { return MakeWidget<PanelWidget>( Ui() ); }

        WidgetBuilder<ScrollContainerWidget> ScrollBox() const { return MakeWidget<ScrollContainerWidget>( Ui() ); }

        // - Leaves

        WidgetBuilder<TextWidget> Text( RatUI::Text a_Text, const TextLayoutStyle& a_Style = {} ) const { return MakeWidget<TextWidget>( Ui(), std::move( a_Text ), a_Style ); }
        WidgetBuilder<TextWidget> Text( String a_String, const TextLayoutStyle& a_Style = {} ) const    { return MakeWidget<TextWidget>( Ui(), RatUI::Text{ std::move( a_String ) }, a_Style ); }

        WidgetBuilder<ButtonWidget> Button( ButtonWidget::OnClickCallback a_OnClick = {} ) const
        {
            return MakeWidget<ButtonWidget>( Ui(), std::move( a_OnClick ) ).ChildAlign( EAlign::Center );
        }

        /** @brief A button with a centered label. */
        WidgetBuilder<ButtonWidget> Button( String a_Label, ButtonWidget::OnClickCallback a_OnClick = {} ) const
        {
            return Button( std::move( a_OnClick ) )(
                Text( std::move( a_Label ) ).NoWrap().Visibility( EVisibility::HitTestInvisible )
            );
        }

        WidgetBuilder<SliderWidget> Slider( f32 a_Min = 0.f, f32 a_Max = 1.f, f32 a_Value = 0.f, EOrient a_Orientation = EOrient::Horizontal ) const
        {
            return MakeWidget<SliderWidget>( Ui(), a_Min, a_Max, a_Value ).Orientation( a_Orientation );
        }

        WidgetBuilder<InputTextWidget> InputText( Unique<ITextEditPolicy> a_Policy = MakeUnique<SingleLineTextEditPolicy>() ) const
        {
            return MakeWidget<InputTextWidget>( Ui(), std::move( a_Policy ) );
        }

        WidgetBuilder<InputTextWidget> TextArea() const { return InputText( MakeUnique<MultiLineTextEditPolicy>() ); }

    private:
        const B& Ui() const { return Detail::SelfOf<B>( this ); }
    };

    using SceneBuilder = BasicSceneBuilder<StandardFactories>;

} // namespace RatUI
