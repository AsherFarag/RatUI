# Changelog

All notable changes to RatUI since the last release are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and RatUI follows
[Semantic Versioning](https://semver.org/). While the version is 0.x, minor
releases may contain breaking API changes.

## [Unreleased]

## [0.2.0] - 2026-11-28

### Added

- `Scene::IsPointerOverUI()` tells the game whether the pointer is the UI's, and `Scene::ClearHover()` tells the scene
  the mouse has left it (the window, or for mouse look). `Scene::IsOver( node, position )`.
- Safe areas: `Scene::SetSafeArea` sets the insets content should avoid (a notch, a home bar, TV overscan), and
  `LayoutNode::SafeAreaEdges` picks the edges a node keeps its content clear of. `SafeAreaInsets( node )`,
  `ContentPadding( node )`.
- Tab and Shift+Tab move focus through every navigation stop in layout order, wrapping at the ends
  (`ENavAction::Next` / `Previous`). A scope pushed with `Scene::PushNavScope` confines them.
- Engaging: a focused widget can be put in an edit mode where it takes the keys navigation would use
  (`WidgetTraits::Engageable`, `Scene::Engage` / `Disengage`, `Reply::Engage` / `Disengage`, `IWidget::OnEngage` /
  `OnDisengage`). Activate engages it; Activate, Cancel or Tab disengage it.
- Sliders are adjusted from the keyboard or a gamepad while engaged, by `Step` or 1% of the range, and draw a focus
  ring (`SliderWidget::FocusRing`).
- `InputTextBaseWidget`: a text field's logic without its look. `InputTextWidget` is its default look; a custom field
  implements `OnPaintField`.
- `LayoutNode::ContentOffset` moves a node's children without running layout, e.g. to scroll them
  (`Scene::SetContentOffset`).
- `Scene::BringIntoView` and `IWidget::OnBringIntoView`. Focus moved by keyboard or gamepad is scrolled into view.
- `ScrollContainerWidget`: `OnScroll` takes a `ScrollEvent` (offset, range, their changes and an `EScrollCause`).
  `ScrollToEnd()`, `StickToEndX` / `StickToEndY`, `GetMaxScrollOffset()`, and `EScrollbarPlacement` (`Gutter` or
  `Overlay`).
- Wheel latching: one wheel gesture stays with the widget that took its first notch
  (`GestureConfig::WheelLatchSeconds`, `WheelLatchDistance`).
- `LayoutResult::ParentRect`, `SceneRect` (replaces `FinalRect`) and `ContentSize`.
- `IWidget::OnGeometryChanged`, called after layout when a watched node's size changed (`LayoutNode::NotifyGeometry`).
- `IWidget::OnActivate`, called when navigation activates the focused widget (Enter, Space, gamepad A). Buttons click
  through it.
- `IWidget::GetContentNodeID`: a widget created under a `ScrollContainerWidget` lands in its scrolled content.
- `IWidget::GetTextInputRect`, `Scene::GetTextInputRect` and `Scene::WantsTextInput`, for placing an IME or on-screen
  keyboard.
- `IWidget::PaintChildren( event, node )` paints the widgets below any node.
- `Scene::SetLayoutScratchSize`. Layout scratch memory is per scene.
- `ShapedGlyph::Cluster`, the text range of each `ShapedLine`, and `ShapedText::CaretOffset`.
- `Unicode::NormalizeWhitespace( text, collapseSpaces, keepLineBreaks )`.

### Changed

- **Breaking:** `WidgetTraits` (`Focus`, `Interactable`, `NavGroup`, `Engageable`, `WidthDependentContent`), passed to
  `IWidget`'s constructor, replaces the `IsFocusable()`, `IsNavigationBoundary()`, `IsInteractable()` and
  `HasWidthDependentContent()` overrides and `LayoutStyle::IsFocusScope`. `SetFocusPolicy`, `SetNavGroup` and
  `SetInteractable` change them per instance.
- **Breaking:** `NavReply::Stop` is now `NavReply::Wrap`, and `NavReply::Custom` is removed (use `Explicit`).
  `OnNavigationBoundary` is asked of the widget whose scope navigation is in. Arrows don't leave a scope pushed with
  `PushNavScope`.
- **Breaking:** key presses go to the focused widget, then its ancestors, then navigation.
- **Breaking:** `Scene::DispatchInputEvent` returns `true` for every pointer event while the pointer is the UI's, and
  `false` for navigation keys that did nothing.
- **Breaking:** a press that landed on no widget is the game's until it is released: no widget hovers or gets its
  moves, wheel or buttons.
- **Breaking:** `Scene::UpdateLayout` does nothing when nothing is dirty. After changing `LayoutNode::Style` directly,
  call `MarkDirty()`.
- **Breaking:** `PanelWidget` is not focusable by default. `SetNavGroup( true )` makes it a group.
- **Breaking:** a text field is edited while engaged: Activate or a click starts editing (`IsEditing`), and Cancel or
  Tab stops. `HasFocus` is removed. Subclasses override `OnPaintField` instead of `OnPaint`, and `GetTextRect` takes
  no argument.
- **Breaking:** scrolling is no longer part of layout: scrolled children's `SceneRect`s are where they are drawn, and
  `LayoutNode::ChildHitTestOffset` is removed. An `Auto` scrollbar keeps its gutter instead of taking and giving back
  layout space, and the horizontal bar defaults to `Never`.
- **Breaking:** `EVisibility` is `Visible`, `Hidden` or `Collapsed`. `HitTestInvisible` becomes
  `LayoutNode::HitTestable( false )`, and `SelfHitTestInvisible` becomes `IWidget::SetInteractable( false )`.
- **Breaking:** `SliderWidget` takes the wheel only while focused. `IWidget::ScrollsContent()` is removed.
- **Breaking:** `TextWrap`'s whitespace and newline settings apply in every break mode. `TextWrap::Prewrap()` is
  removed.
- **Breaking:** `NodeID` is 64 bits (a 32-bit index and a 32-bit version), so the ID of a destroyed widget stays
  invalid.
- **Breaking:** `InputState::LastPointerEvent` is replaced by `InputState::Hover`.
- **Breaking:** `Scene` can't be moved.
- **Breaking:** `Scene::DestroyWidget` returns `void`, and `Reply::PreventDefault` is removed.
- Focus moves when a pointer presses, not when it is released. Pressing empty space clears focus.
- Arrow keys consider every focusable widget in the current scope, not only its direct children. With nothing focused,
  the first arrow or Tab only picks a widget.
- `ScrollContainerWidget` and its scrollbars aren't focusable. The wheel scrolls it from anywhere over its content.
- A scroll container doesn't paint children scrolled out of view.
- A horizontal `SliderWidget` takes the vertical wheel when the event has no sideways scroll.
- `Scene::UpdateLayout` runs up to four passes, and re-hit-tests hover instead of replaying the last pointer event.
- A press of a pointer button that is already down cancels the old press.
- `InputTextWidget` places its caret from the shaped glyphs, and ignores `TextLayoutStyle::Transform` and `Wrap`.
- Trailing spaces on a line are shaped, and hang past the line's width.
- `Edges::Apply` returns a zero-sized rect instead of one with negative size.
- `Builder::WithDebugName` compiles in release, where it does nothing.
- `ITextEditPolicy::HandleText` is virtual.

### Fixed

- Destroying a widget destroys all of its descendants, not only its first child.
- Destroying the focused or engaged widget clears focus and its nav scopes.
- Widgets under a bare layout node (`Scene::CreateLayoutNode`) are painted.
- Creating a widget or layout node under a parent that no longer exists asserts.
- A button no longer clicks when the pointer moves off it or is released elsewhere.
- Hover follows a captured pointer, and goes to what is under the pointer when the capture ends.
- Pressing another button during a drag no longer takes the pointer from the widget being dragged.
- A slider or scrollbar thumb grabbed off its centre doesn't jump to the pointer.
- `SliderWidget::Step` snaps the value when dragging, for any range.
- `SliderWidget` no longer crashes when painted in MSVC 19.52 Release builds.
- In Horizontal, Vertical and Grid layouts a child's margin sits around it instead of shrinking it.
- `SizeConstraints` hold for flex and percent children of overlay and grid layouts.
- `LayoutResult::ContentSize` counts start padding and the children's trailing margins.
- A Collapsed root no longer runs layout on every `UpdateLayout`, and everything below a Collapsed node is collapsed at
  once.
- Content that grows at its final width (e.g. wrapped text) grows the content-sized nodes around it.
- A root that is switched, or a child that is added or removed, is laid out again.
- A widget focused before it is laid out is brought into view afterwards.
- `ScrollContainerWidget` keeps a scroll offset set before layout, and dragging its scrollbar scrolls the content.
- `DrawList::PushClipRect` applies the current transform.
- `TextWidget` clips text that overflows its box in `ETextOverflow::Clip` mode.
- A nine-slice keeps its size as it moves, instead of gaining or losing a pixel.
- Mouse button events without a pointer id count as the mouse.
- `Scene::UpdateLayout` no longer replays a lifted touch.
- `Reply::Engage` no longer overwrites the target of `Reply::SetFocus`.
- `InputNavMap::BindDefaultGamepad` moves left and right with the D-pad.
- `NumericTextEditPolicy` accepts a minus sign in floating-point fields.
- Wrapped text no longer drops a word that is kept with the punctuation after it.
- FreeType shaping no longer repeats the previous line's glyphs on a blank line.
- `TextWrap::PreWrap` keeps spaces at the start of a line, and a line break right after spaces is kept.
- `TextWrap::Pre()` keeps its spaces and line breaks.

## [0.1.1] - 2026-10-02

### Added

- `RatUI::Visit` for variant visitation through `CoreTraits` (`std::visit` by default).
- `RatUI::FindValue( map, key )`: returns a pointer to the mapped value or `nullptr`, so callers don't depend on `std::pair`'s `->second`.
- `OPTIMIZE_DEBUG` option for `ratui_require_dependency`, building a fetched dependency optimized in Debug while keeping it link-compatible.

### Changed

- All container and variant access inside RatUI now goes through `CoreTraits` and the generic container functions
  (`Visit` / `Holds` / `Get`, `Find`, `Size`, ...) instead of std member functions and helpers, so containers overridden in
  `RatUIContainerImpl.h` no longer need to mirror the std API.
- `HashMap` forwards its optional parameters to `HashMapImpl` instead of reading std's `::hasher` / `::key_equal` typedefs.
- msdfgen is now built optimized in Debug builds, making text atlas generation usable in Debug.

### Fixed

- The string view override macro is now `RATUI_OVERRIDE_STRING_VIEW_IMPL` (was `RATUI_STRING_VIEW_IMPL`),
  matching the other override macros; the docs now name the real override macros.

[0.2.0]: https://github.com/AsherFarag/RatUI/compare/v0.1.1...v0.2.0
[0.1.1]: https://github.com/AsherFarag/RatUI/compare/v0.1.0...v0.1.1
