/*
*******************************************************************************
* Copyright (C) 1996-1999, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*/

/*
* File coleitr.cpp
*
* 
*
* Created by: Helena Shih
*
* Modification History:
*
*  Date      Name        Description
*
*  6/23/97   helena      Adding comments to make code more readable.
* 08/03/98   erm         Synched with 1.2 version of CollationElementIterator.java
* 12/10/99   aliu        Ported Thai collation support from Java.
* 01/25/01   swquek      Modified to a C++ wrapper calling C APIs (ucoliter.h)
* 02/19/01   swquek      Removed CollationElementsIterator() since it is 
*                        private constructor and no calls are made to it
*/

#include "unicode/coleitr.h"
#include "ucol_imp.h"
#include "cmemory.h"


/* Constants --------------------------------------------------------------- */

/* synwee : public can't remove */
int32_t const CollationElementIterator::NULLORDER = 0xffffffff;

/* CollationElementIterator public constructor/destructor ------------------ */

CollationElementIterator::CollationElementIterator(
                                         const CollationElementIterator& other) 
                                         : isDataOwned_(TRUE)
{
  UErrorCode status = U_ZERO_ERROR;
  m_data_ = ucol_openElements(other.m_data_->iteratordata_.coll, NULL, 0, &status);
  *this = other;
}

CollationElementIterator::~CollationElementIterator()
{
  if (isDataOwned_) {
    ucol_closeElements(m_data_);
  }
}

/* CollationElementIterator public methods --------------------------------- */

UTextOffset CollationElementIterator::getOffset() const
{
  return ucol_getOffset(m_data_);
}

/**
* Get the ordering priority of the next character in the string.
* @return the next character's ordering. Returns NULLORDER if an error has 
*         occured or if the end of string has been reached
*/
int32_t CollationElementIterator::next(UErrorCode& status)
{
  return ucol_next(m_data_, &status);
}

UBool CollationElementIterator::operator!=(
                                  const CollationElementIterator& other) const
{
  return !(*this == other);
}

UBool CollationElementIterator::operator==(
                                    const CollationElementIterator& that) const
{
  if (this == &that)
    return TRUE;
  
  if (m_data_ == that.m_data_)
    return TRUE;
  
  return (this->m_data_->normalization_ == that.m_data_->normalization_ &&
    this->m_data_->length_ == that.m_data_->length_ &&
    this->m_data_->reset_  == that.m_data_->reset_ &&
    uprv_memcmp(this->m_data_->iteratordata_.string, 
                that.m_data_->iteratordata_.string,
                this->m_data_->length_) == 0 &&
    this->getOffset() == that.getOffset() &&  
    this->m_data_->iteratordata_.isThai == that.m_data_->iteratordata_.isThai &&
    this->m_data_->iteratordata_.coll == that.m_data_->iteratordata_.coll);
}

/**
* Get the ordering priority of the previous collation element in the string.
* @param status the error code status.
* @return the previous element's ordering. Returns NULLORDER if an error has 
*         occured or if the start of string has been reached.
*/
int32_t CollationElementIterator::previous(UErrorCode& status)
{
  return ucol_previous(m_data_, &status);
}

/**
* Resets the cursor to the beginning of the string.
*/
void CollationElementIterator::reset()
{
  ucol_reset(m_data_);
}

void CollationElementIterator::setOffset(UTextOffset newOffset, 
                                         UErrorCode& status)
{
  ucol_setOffset(m_data_, newOffset, &status);
}

/**
* Sets the source to the new source string.
*/
void CollationElementIterator::setText(const UnicodeString& source,
                                       UErrorCode& status)
{
  if (U_FAILURE(status))
    return;
  int32_t length = source.length();
  UChar *string = new UChar[length];
  source.extract(0, length, string);
	
  m_data_->length_ = length;

  if (m_data_->iteratordata_.isWritable && 
      m_data_->iteratordata_.string != NULL)
    uprv_free(m_data_->iteratordata_.string);
  init_collIterate(m_data_->iteratordata_.coll, string, length, &m_data_->iteratordata_, TRUE);
}

// Sets the source to the new character iterator.
void CollationElementIterator::setText(CharacterIterator& source, 
                                       UErrorCode& status)
{
  if (U_FAILURE(status)) 
    return;
    
  int32_t length = source.getLength();
  UChar *buffer = new UChar[length];
  /* 
  Using this constructor will prevent buffer from being removed when
  string gets removed
  */
  UnicodeString string;
  source.getText(string);
  string.extract(0, length, buffer);
  m_data_->length_ = length;

  if (m_data_->iteratordata_.isWritable && 
      m_data_->iteratordata_.string != NULL)
    uprv_free(m_data_->iteratordata_.string);
  init_collIterate(m_data_->iteratordata_.coll, buffer, length, &m_data_->iteratordata_, TRUE);
}

int32_t CollationElementIterator::strengthOrder(int32_t order) const
{
  UCollationStrength s = ucol_getStrength(m_data_->iteratordata_.coll);
  // Mask off the unwanted differences.
  if (s == UCOL_PRIMARY)
    order &= RuleBasedCollator::PRIMARYDIFFERENCEONLY;
  else 
    if (s == UCOL_SECONDARY)
      order &= RuleBasedCollator::SECONDARYDIFFERENCEONLY;
    
  return order;
}

/* CollationElementIterator private constructors/destructors --------------- */

/* 
This private method will never be called, but it makes the linker happy
CollationElementIterator::CollationElementIterator() : m_data_(0)
{
}
*/

CollationElementIterator::CollationElementIterator(
                                              const RuleBasedCollator* order)
                                              : isDataOwned_(TRUE)
{
  UErrorCode status = U_ZERO_ERROR;
  m_data_ = ucol_openElements(order->ucollator, NULL, 0, &status);
}

/** 
* This is the "real" constructor for this class; it constructs an iterator
* over the source text using the specified collator
*/
CollationElementIterator::CollationElementIterator(
                                               const UnicodeString& sourceText,
                                               const RuleBasedCollator* order,
                                               UErrorCode& status)
                                               : isDataOwned_(TRUE)
{
  if (U_FAILURE(status))
    return;
 
  int32_t length = sourceText.length();
  UChar *string = new UChar[length];
  /* 
  Using this constructor will prevent buffer from being removed when
  string gets removed
  */
  sourceText.extract(0, length, string);

  m_data_ = ucol_openElements(order->ucollator, string, length, &status);
  m_data_->iteratordata_.isWritable = TRUE;
}

/** 
* This is the "real" constructor for this class; it constructs an iterator over 
* the source text using the specified collator
*/
CollationElementIterator::CollationElementIterator(
                                           const CharacterIterator& sourceText,
                                           const RuleBasedCollator* order,
                                           UErrorCode& status)
                                           : isDataOwned_(TRUE)
{
  if (U_FAILURE(status))
    return;
    
  // **** should I just drop this test? ****
  /*
  if ( sourceText.endIndex() != 0 )
  {
    // A CollationElementIterator is really a two-layered beast.
    // Internally it uses a Normalizer to munge the source text into a form 
    // where all "composed" Unicode characters (such as �) are split into a 
    // normal character and a combining accent character.  
    // Afterward, CollationElementIterator does its own processing to handle
    // expanding and contracting collation sequences, ignorables, and so on.
    
    Normalizer::EMode decomp = order->getStrength() == Collator::IDENTICAL
                               ? Normalizer::NO_OP : order->getDecomposition();
      
    text = new Normalizer(sourceText, decomp);
    if (text == NULL)
      status = U_MEMORY_ALLOCATION_ERROR;    
  }
  */
  int32_t length = sourceText.getLength();
  UChar *buffer = new UChar[length];
  /* 
  Using this constructor will prevent buffer from being removed when
  string gets removed
  */
  UnicodeString string(buffer, length, length);
  ((CharacterIterator &)sourceText).getText(string);
  string.extract(0, length, buffer);
  
  m_data_ = ucol_openElements(order->ucollator, buffer, length, &status);
  m_data_->iteratordata_.isWritable = TRUE;
}

/* CollationElementIterator private methods -------------------------------- */

const CollationElementIterator& CollationElementIterator::operator=(
                                         const CollationElementIterator& other)
{
  if (this != &other)
  {
    this->m_data_->normalization_ = other.m_data_->normalization_;
    this->m_data_->length_        = other.m_data_->length_;
    this->m_data_->reset_         = other.m_data_->reset_;
    

    this->m_data_->iteratordata_.string   = other.m_data_->iteratordata_.string;
    this->m_data_->iteratordata_.start    = other.m_data_->iteratordata_.start;
    this->m_data_->iteratordata_.len      = other.m_data_->iteratordata_.len;
    this->m_data_->iteratordata_.pos      = other.m_data_->iteratordata_.pos;
    this->m_data_->iteratordata_.toReturn = other.m_data_->iteratordata_.CEs + 
          (other.m_data_->iteratordata_.toReturn - other.m_data_->iteratordata_.CEs);
    this->m_data_->iteratordata_.CEpos    = other.m_data_->iteratordata_.CEs + 
          (other.m_data_->iteratordata_.CEpos - other.m_data_->iteratordata_.CEs);
    uprv_memcpy(this->m_data_->iteratordata_.CEs, other.m_data_->iteratordata_.CEs, 
                UCOL_EXPAND_CE_BUFFER_SIZE * sizeof(uint32_t));
    this->m_data_->iteratordata_.isThai   = other.m_data_->iteratordata_.isThai;
    this->m_data_->iteratordata_.isWritable = other.m_data_->iteratordata_.isWritable;

    uprv_memcpy(this->m_data_->iteratordata_.stackWritableBuffer, 
                other.m_data_->iteratordata_.stackWritableBuffer, 
                UCOL_WRITABLE_BUFFER_SIZE * sizeof(UChar));
    /* writablebuffer is not used at the moment, not used */
    this->m_data_->iteratordata_.coll = other.m_data_->iteratordata_.coll;
    this->isDataOwned_ = FALSE;
  }

  return *this;
}


