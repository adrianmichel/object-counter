/*
Copyright (C) 2018 Adrian Michel
http://www.amichel.com

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#ifdef _DEBUG

#include <fstream>
#include <set>

#include <boost/log/trivial.hpp>

namespace amichel {
	class Counter {
	private:
		int m_count;
		std::string m_name;
		std::set<int> m_indexes;
		// total number of instances created
		int m_total;
		int m_max;
		amichel::Mutex m_mx;

		void setName(const std::string& name) {
			if (m_name.empty()) m_name = name;
		}

	public:
		Counter() : m_count(0), m_total(0), m_max(0) {}

		~Counter() {
			amichel::Lock lock(m_mx);
			assert(m_indexes.size() == m_count);
			if (m_count > 0) {
				BOOST_LOG_TRIVIAL(error) << "ObjCounter - leaks found:" << std::endl << m_name << "leak\n\tTotal created:\t " << m_total
					<< "\n\tMax created:\t " << m_max
					<< "\n\tLeaked count:\t " << m_count
					<< "\n\tLeaked indexes:\t "
					<< leakedIxsAsString();
			}
			else if (m_count < 0)
				assert(false);
			else {
				// reached count 0 - all instances have been destroyed
				// todo: we may want to log this when  debugging memory/object leaks
			}
		}

		int inc(const std::string& name, int breakIndex) {
			amichel::Lock lock(m_mx);
			setName(name);
			++m_total;
			++m_count;
			// latest removed is used to ensure that we don't get a collision
			// in case an earlier index is removed before a later index
			int ix = m_total;

			m_max = m_total;
			m_indexes.insert(ix);
			if (ix == breakIndex) {
				DebugBreak();
			}
			BOOST_LOG_TRIVIAL(debug) << "ObjCounter - adding \"" << m_name << "\", index " << ix
				<< ", total " << m_total;
			return ix;
		}

		void removeIndex(int index) {
			amichel::Lock lock(m_mx);
			--m_count;
			if (m_indexes.find(index) == m_indexes.end()) {
				// this indes has already been deleted - error
				assert(false);
				//			BOOST_LOG_TRIVIAL( log_debug, "ObjCounter - \"" << m_name <<
				//"\" attempting to remove not found index (deleting twice?)\n\tindex:\t "
				//<< index << "\n\tcurrent ix map size:\t" << m_indexes.size() );
			}
			else {
				m_indexes.erase(index);
				BOOST_LOG_TRIVIAL(debug) << "ObjCounter - removing \"" << m_name << "\", index "
					<< index << ", remaining "
					<< m_indexes.size();
			}
		}

	private:
#define MAX_LEAKED_INDEXES_DISPLAYED 20
		std::string leakedIxsAsString() {
			std::string leakedIxs;
			int n = 0;
			for (std::set<int>::const_iterator i = m_indexes.begin();
				i != m_indexes.end() && n < MAX_LEAKED_INDEXES_DISPLAYED; ++i, ++n)
				leakedIxs << (n == 0 ? "" : ", ") << *i;

			if (n >= MAX_LEAKED_INDEXES_DISPLAYED) leakedIxs << ", ...";

			return leakedIxs;
		}
	};

	template <typename T>
	class ObjCounterBase {
		int m_objIx;
		static Counter m_count;

	public:
		ObjCounterBase(const std::string& name, int breakIndex) {
			m_objIx = m_count.inc(name, breakIndex);
		}

		// this is to make sure that when assigning an object of one class to another
		// it won't change the object index (otherwise the default operator= would set
		// the m_objIx)
		ObjCounterBase<T>& operator=(const ObjCounterBase& c) { return *this; }

		virtual ~ObjCounterBase() { m_count.removeIndex(m_objIx); }
	};

}  // namespace amichel

#define OBJ_COUNTER_NAME(COUNTER_STR) ObjCounter##COUNTER_STR

template <typename T>
amichel::Counter amichel::ObjCounterBase<T>::m_count;

#define OBJ_CTR(COUNTER_STR, BREAK_INDEX)                         \
  \
\
class OBJ_COUNTER_NAME(COUNTER_STR)                               \
      : public amichel::ObjCounterBase<COUNTER_STR>\
{             \
          \
public : OBJ_COUNTER_NAME(COUNTER_STR)() :                        \
              amichel::ObjCounterBase<COUNTER_STR>(#COUNTER_STR,  \
                                                   BREAK_INDEX){} \
        \
};

#define OBJ_COUNTER(COUNTER_STR) \
  \
OBJ_CTR(COUNTER_STR, 0)          \
  \
OBJ_COUNTER_NAME(COUNTER_STR)    \
  m_objCounter;

#define OBJ_COUNTER_BREAK_ON_INDEX(COUNTER_STR, BREAK_INDEX) \
  \
OBJ_CTR(COUNTER_STR, BREAK_INDEX)                            \
  \
OBJ_COUNTER_NAME(COUNTER_STR)                                \
  m_objCounter;

#else
#define OBJ_COUNTER(COUNTER_STR)
#define OBJ_COUNTER_BREAK_ON_INDEX(COUNTER_STR, BREAK_INDEX)
#endif  //_DEBUG
