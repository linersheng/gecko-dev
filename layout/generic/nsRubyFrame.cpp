/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code is subject to the terms of the Mozilla Public License
 * version 2.0 (the "License"). You can obtain a copy of the License at
 * http://mozilla.org/MPL/2.0/. */

/* rendering object for CSS "display: ruby" */
#include "nsRubyFrame.h"
#include "nsLineLayout.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "WritingModes.h"
#include "RubyUtils.h"
#include "RubyReflowState.h"
#include "nsRubyBaseContainerFrame.h"
#include "nsRubyTextContainerFrame.h"

using namespace mozilla;

//----------------------------------------------------------------------

// Frame class boilerplate
// =======================

NS_QUERYFRAME_HEAD(nsRubyFrame)
  NS_QUERYFRAME_ENTRY(nsRubyFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

NS_IMPL_FRAMEARENA_HELPERS(nsRubyFrame)

nsContainerFrame*
NS_NewRubyFrame(nsIPresShell* aPresShell,
                nsStyleContext* aContext)
{
  return new (aPresShell) nsRubyFrame(aContext);
}

//----------------------------------------------------------------------

// nsRubyFrame Method Implementations
// ==================================

nsIAtom*
nsRubyFrame::GetType() const
{
  return nsGkAtoms::rubyFrame;
}

/* virtual */ bool
nsRubyFrame::IsFrameOfType(uint32_t aFlags) const
{
  return nsContainerFrame::IsFrameOfType(aFlags &
    ~(nsIFrame::eLineParticipant));
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsRubyFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Ruby"), aResult);
}
#endif

/**
 * This enumerator enumerates each segment.
 */
class MOZ_STACK_CLASS SegmentEnumerator
{
public:
  explicit SegmentEnumerator(nsRubyFrame* aRubyFrame);

  void Next();
  bool AtEnd() const { return !mBaseContainer; }

  nsRubyBaseContainerFrame* GetBaseContainer() const
  {
    return mBaseContainer;
  }

private:
  nsRubyBaseContainerFrame* mBaseContainer;
};

SegmentEnumerator::SegmentEnumerator(nsRubyFrame* aRubyFrame)
{
  nsIFrame* frame = aRubyFrame->GetFirstPrincipalChild();
  MOZ_ASSERT(!frame ||
             frame->GetType() == nsGkAtoms::rubyBaseContainerFrame);
  mBaseContainer = static_cast<nsRubyBaseContainerFrame*>(frame);
}

void
SegmentEnumerator::Next()
{
  MOZ_ASSERT(mBaseContainer);
  nsIFrame* frame = mBaseContainer->GetNextSibling();
  while (frame && frame->GetType() != nsGkAtoms::rubyBaseContainerFrame) {
    frame = frame->GetNextSibling();
  }
  mBaseContainer = static_cast<nsRubyBaseContainerFrame*>(frame);
}

/* virtual */ void
nsRubyFrame::AddInlineMinISize(nsRenderingContext *aRenderingContext,
                               nsIFrame::InlineMinISizeData *aData)
{
  nscoord max = 0;
  for (SegmentEnumerator e(this); !e.AtEnd(); e.Next()) {
    max = std::max(max, e.GetBaseContainer()->GetMinISize(aRenderingContext));
  }
  aData->currentLine += max;
}

/* virtual */ void
nsRubyFrame::AddInlinePrefISize(nsRenderingContext *aRenderingContext,
                                nsIFrame::InlinePrefISizeData *aData)
{
  nscoord sum = 0;
  for (SegmentEnumerator e(this); !e.AtEnd(); e.Next()) {
    sum += e.GetBaseContainer()->GetPrefISize(aRenderingContext);
  }
  aData->currentLine += sum;
}

/* virtual */ LogicalSize
nsRubyFrame::ComputeSize(nsRenderingContext *aRenderingContext,
                           WritingMode aWM,
                           const LogicalSize& aCBSize,
                           nscoord aAvailableISize,
                           const LogicalSize& aMargin,
                           const LogicalSize& aBorder,
                           const LogicalSize& aPadding,
                           ComputeSizeFlags aFlags)
{
  // Ruby frame is inline, hence don't compute size before reflow.
  return LogicalSize(aWM, NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
}

/* virtual */ nscoord
nsRubyFrame::GetLogicalBaseline(WritingMode aWritingMode) const
{
  return mBaseline;
}

/* virtual */ bool
nsRubyFrame::CanContinueTextRun() const
{
  return true;
}

/* virtual */ void
nsRubyFrame::Reflow(nsPresContext* aPresContext,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsRubyFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  
  if (!aReflowState.mLineLayout) {
    NS_ASSERTION(aReflowState.mLineLayout,
                 "No line layout provided to RubyFrame reflow method.");
    aStatus = NS_FRAME_COMPLETE;
    return;
  }

  // Grab overflow frames from prev-in-flow and its own.
  MoveOverflowToChildList();

  // Begin the span for the ruby frame
  WritingMode frameWM = aReflowState.GetWritingMode();
  WritingMode lineWM = aReflowState.mLineLayout->GetWritingMode();
  LogicalMargin borderPadding = aReflowState.ComputedLogicalBorderPadding();
  nscoord startEdge = borderPadding.IStart(frameWM);
  nscoord endEdge = aReflowState.AvailableISize() - borderPadding.IEnd(frameWM);
  NS_ASSERTION(aReflowState.AvailableISize() != NS_UNCONSTRAINEDSIZE,
               "should no longer use available widths");
  aReflowState.mLineLayout->BeginSpan(this, &aReflowState,
                                      startEdge, endEdge, &mBaseline);

  aStatus = NS_FRAME_COMPLETE;
  for (SegmentEnumerator e(this); !e.AtEnd(); e.Next()) {
    ReflowSegment(aPresContext, aReflowState, e.GetBaseContainer(), aStatus);

    if (NS_INLINE_IS_BREAK(aStatus)) {
      // A break occurs when reflowing the segment.
      // Don't continue reflowing more segments.
      break;
    }
  }

  ContinuationTraversingState pullState(this);
  while (aStatus == NS_FRAME_COMPLETE) {
    nsRubyBaseContainerFrame* baseContainer = PullOneSegment(pullState);
    if (!baseContainer) {
      // No more continuations after, finish now.
      break;
    }
    ReflowSegment(aPresContext, aReflowState, baseContainer, aStatus);
  }
  // We never handle overflow in ruby.
  MOZ_ASSERT(!NS_FRAME_OVERFLOW_IS_INCOMPLETE(aStatus));

  aDesiredSize.ISize(lineWM) = aReflowState.mLineLayout->EndSpan(this);
  nsLayoutUtils::SetBSizeFromFontMetrics(this, aDesiredSize, aReflowState,
                                         borderPadding, lineWM, frameWM);
}

#ifdef DEBUG
static void
SanityCheckRubyPosition(int8_t aRubyPosition)
{
  uint8_t horizontalPosition = aRubyPosition &
    (NS_STYLE_RUBY_POSITION_LEFT | NS_STYLE_RUBY_POSITION_RIGHT);
  MOZ_ASSERT(horizontalPosition == NS_STYLE_RUBY_POSITION_LEFT ||
             horizontalPosition == NS_STYLE_RUBY_POSITION_RIGHT);
  uint8_t verticalPosition = aRubyPosition &
    (NS_STYLE_RUBY_POSITION_OVER | NS_STYLE_RUBY_POSITION_UNDER |
     NS_STYLE_RUBY_POSITION_INTER_CHARACTER);
  MOZ_ASSERT(verticalPosition == NS_STYLE_RUBY_POSITION_OVER ||
             verticalPosition == NS_STYLE_RUBY_POSITION_UNDER ||
             verticalPosition == NS_STYLE_RUBY_POSITION_INTER_CHARACTER);
}
#endif

void
nsRubyFrame::ReflowSegment(nsPresContext* aPresContext,
                           const nsHTMLReflowState& aReflowState,
                           nsRubyBaseContainerFrame* aBaseContainer,
                           nsReflowStatus& aStatus)
{
  WritingMode lineWM = aReflowState.mLineLayout->GetWritingMode();
  LogicalSize availSize(lineWM, aReflowState.AvailableISize(),
                        aReflowState.AvailableBSize());

  nsAutoTArray<nsRubyTextContainerFrame*, RTC_ARRAY_SIZE> textContainers;
  for (RubyTextContainerIterator iter(aBaseContainer); !iter.AtEnd(); iter.Next()) {
    textContainers.AppendElement(iter.GetTextContainer());
  }
  const uint32_t rtcCount = textContainers.Length();
  RubyReflowState rubyReflowState(lineWM, textContainers);

  nsHTMLReflowMetrics baseMetrics(aReflowState);
  bool pushedFrame;
  aReflowState.mLineLayout->SetRubyReflowState(&rubyReflowState);
  aReflowState.mLineLayout->ReflowFrame(aBaseContainer, aStatus,
                                        &baseMetrics, pushedFrame);

  if (NS_INLINE_IS_BREAK_BEFORE(aStatus)) {
    if (aBaseContainer != mFrames.FirstChild()) {
      // Some segments may have been reflowed before, hence it is not
      // a break-before for the ruby container.
      aStatus = NS_INLINE_LINE_BREAK_AFTER(NS_FRAME_NOT_COMPLETE);
      PushChildren(aBaseContainer, aBaseContainer->GetPrevSibling());
      aReflowState.mLineLayout->SetDirtyNextLine();
    }
    // This base container is not placed at all, we can skip all
    // text containers paired with it.
    return;
  }
  if (NS_FRAME_IS_NOT_COMPLETE(aStatus)) {
    // It always promise that if the status is incomplete, there is a
    // break occurs. Break before has been processed above. However,
    // it is possible that break after happens with the frame reflow
    // completed. It happens if there is a force break at the end.
    MOZ_ASSERT(NS_INLINE_IS_BREAK_AFTER(aStatus));
    // Find the previous sibling which we will
    // insert new continuations after.
    nsIFrame* lastChild;
    if (rtcCount > 0) {
      lastChild = textContainers.LastElement();
    } else {
      lastChild = aBaseContainer;
    }

    // Create continuations for the base container
    nsIFrame* newBaseContainer = CreateNextInFlow(aBaseContainer);
    // newBaseContainer is null if there are existing next-in-flows.
    // We only need to move and push if there were not.
    if (newBaseContainer) {
      // Move the new frame after all the text containers
      mFrames.RemoveFrame(newBaseContainer);
      mFrames.InsertFrame(nullptr, lastChild, newBaseContainer);

      // Create continuations for text containers
      nsIFrame* newLastChild = newBaseContainer;
      for (uint32_t i = 0; i < rtcCount; i++) {
        nsIFrame* newTextContainer = CreateNextInFlow(textContainers[i]);
        MOZ_ASSERT(newTextContainer, "Next-in-flow of rtc should not exist "
                   "if the corresponding rbc does not");
        mFrames.RemoveFrame(newTextContainer);
        mFrames.InsertFrame(nullptr, newLastChild, newTextContainer);
        newLastChild = newTextContainer;
      }
      PushChildren(newBaseContainer, lastChild);
      aReflowState.mLineLayout->SetDirtyNextLine();
    }
  } else {
    // If the ruby base container is reflowed completely, the line
    // layout will remove the next-in-flows of that frame. But the
    // line layout is not aware of the ruby text containers, hence
    // it is necessary to remove them here.
    for (uint32_t i = 0; i < rtcCount; i++) {
      nsIFrame* nextRTC = textContainers[i]->GetNextInFlow();
      if (nextRTC) {
        nextRTC->GetParent()->DeleteNextInFlowChild(nextRTC, true);
      }
    }
  }

  nsRect baseRect = aBaseContainer->GetRect();
  // We need to position our rtc frames on one side or the other of the
  // base container's rect, using a coordinate space that's relative to
  // the ruby frame. Right now, the base container's rect's block-axis
  // position is relative to the block container frame containing the
  // lines, so we use 0 instead. (i.e. we assume that the base container
  // is adjacent to the ruby frame's block-start edge.)
  // XXX We may need to add border/padding here. See bug 1055667.
  (lineWM.IsVertical() ? baseRect.x : baseRect.y) = 0;
  // The rect for offsets of text containers.
  nsRect offsetRect = baseRect;
  for (uint32_t i = 0; i < rtcCount; i++) {
    nsRubyTextContainerFrame* textContainer = textContainers[i];
    rubyReflowState.AdvanceCurrentContainerIndex();

    nsReflowStatus textReflowStatus;
    nsHTMLReflowMetrics textMetrics(aReflowState);
    nsHTMLReflowState textReflowState(aPresContext, aReflowState,
                                      textContainer, availSize);
    textReflowState.mRubyReflowState = &rubyReflowState;
    // FIXME We probably shouldn't be using the same nsLineLayout for
    //       the text containers. But it should be fine now as we are
    //       not actually using this line layout to reflow something,
    //       but just read the writing mode from it.
    textReflowState.mLineLayout = aReflowState.mLineLayout;
    textContainer->Reflow(aPresContext, textMetrics,
                          textReflowState, textReflowStatus);
    // Ruby text containers always return NS_FRAME_COMPLETE even when
    // they have continuations, because the breaking has already been
    // handled when reflowing the base containers.
    NS_ASSERTION(textReflowStatus == NS_FRAME_COMPLETE,
                 "Ruby text container must not break itself inside");
    textContainer->SetSize(LogicalSize(lineWM, textMetrics.ISize(lineWM),
                                       textMetrics.BSize(lineWM)));

    nscoord x, y;
    nscoord bsize = textMetrics.BSize(lineWM);
    uint8_t rubyPosition = textContainer->StyleText()->mRubyPosition;
#ifdef DEBUG
    SanityCheckRubyPosition(rubyPosition);
#endif
    if (lineWM.IsVertical()) {
      // writing-mode is vertical, so bsize is the annotation's *width*
      if (rubyPosition & NS_STYLE_RUBY_POSITION_LEFT) {
        x = offsetRect.X() - bsize;
        offsetRect.SetLeftEdge(x);
      } else {
        x = offsetRect.XMost();
        offsetRect.SetRightEdge(x + bsize);
      }
      y = offsetRect.Y();
    } else {
      // writing-mode is horizontal, so bsize is the annotation's *height*
      x = offsetRect.X();
      if (rubyPosition & NS_STYLE_RUBY_POSITION_OVER) {
        y = offsetRect.Y() - bsize;
        offsetRect.SetTopEdge(y);
      } else if (rubyPosition & NS_STYLE_RUBY_POSITION_UNDER) {
        y = offsetRect.YMost();
        offsetRect.SetBottomEdge(y + bsize);
      } else {
        // XXX inter-character support in bug 1055672
        MOZ_ASSERT_UNREACHABLE("Unsupported ruby-position");
        y = offsetRect.Y();
      }
    }
    FinishReflowChild(textContainer, aPresContext, textMetrics,
                      &textReflowState, x, y, 0);
  }
}

nsRubyBaseContainerFrame*
nsRubyFrame::PullOneSegment(ContinuationTraversingState& aState)
{
  // Pull a ruby base container
  nsIFrame* baseFrame = PullNextInFlowChild(aState);
  if (!baseFrame) {
    return nullptr;
  }
  MOZ_ASSERT(baseFrame->GetType() == nsGkAtoms::rubyBaseContainerFrame);

  // Pull all ruby text containers following the base container
  nsIFrame* nextFrame;
  while ((nextFrame = GetNextInFlowChild(aState)) != nullptr &&
         nextFrame->GetType() == nsGkAtoms::rubyTextContainerFrame) {
    PullNextInFlowChild(aState);
  }

  return static_cast<nsRubyBaseContainerFrame*>(baseFrame);
}
