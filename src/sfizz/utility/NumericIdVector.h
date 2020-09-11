// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "utility/NumericId.h"
/**
 * @brief A resizable vector using NumericId (thus possibly negative) indices
 *
 * @tparam T The contained object
 * @tparam U The template of the NumericId indices
 */
template<class T, class U>
class NumericIdVector
{
public:
    /**
     * @brief Resize the vector if necessary
     *
     * @param id
     */
    void ensureSize(const NumericId<U>& id)
    {
        if (id.number() < 0) {
            if (negativeIndices_.size() <= static_cast<size_t>(-id.number()))
                negativeIndices_.resize(-id.number());
        } else {
            if (positiveIndices_.size() <= static_cast<size_t>(id.number() + 1))
                positiveIndices_.resize(id.number() + 1);
        }
    }

    /**
     * @brief Check that the underlying containers actually contain a (possibly default constructed)
     * element at this id.
     *
     * @param id
     * @return true
     * @return false
     */
    bool validId(const NumericId<U>& id) {
        if (id.number() < 0) {
            if (negativeIndices_.size() <= static_cast<size_t>(-id.number()))
                return false;
        } else {
            if (positiveIndices_.size() <= static_cast<size_t>(id.number() + 1))
                return false;
        }

        return true;
    }

    /**
     * @brief Clear the underlying containers
     *
     */
    void clear()
    {
        positiveIndices_.clear();
        negativeIndices_.clear();
    }

    /**
     * @brief Access an element at a given id
     *
     * @param id
     * @return T&
     */
    T& operator[](const NumericId<U>& id)
    {
        if (id.number() < 0) {
            return negativeIndices_[-id.number() - 1];
        } else {
            return positiveIndices_[id.number()];
        }
    }

    const T& operator[](const NumericId<U>& id) const { return operator[](id); }

private:
    std::vector<T> positiveIndices_;
    std::vector<T> negativeIndices_;
};
