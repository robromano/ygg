//
// Created by lukas on 31.08.17.
//

#ifndef YGG_INTERVALMAP_HPP
#define YGG_INTERVALMAP_HPP

#include "intervaltree.hpp"
#include "rbtree.hpp"

namespace ygg {

namespace internal {
	template<class KeyT, class ValueT, int Tag>
	class InnerNode : public  RBTreeNodeBase<InnerNode<KeyT, ValueT, Tag>,
	                                                   TreeOptions<TreeFlags::MULTIPLE>, Tag> {
	public:
		KeyT point;
		ValueT aggregate;


		class Compare {
		public:
			constexpr bool operator()(const InnerNode<KeyT, ValueT, Tag> & lhs,
			                          const InnerNode<KeyT, ValueT, Tag> & rhs) const
			{
				return lhs.point < rhs.point;
			}

			constexpr bool operator()(int lhs,
			                          const InnerNode<KeyT, ValueT, Tag> & rhs) const
			{
				return lhs < rhs.point;
			}

			constexpr bool operator()(const InnerNode<KeyT, ValueT, Tag> & lhs,
			                          int rhs) const
			{
				return lhs.point < rhs;
			}
		};
	};



} // namespace internal

/**
 * @brief Base class (template) to supply your node class with metainformation for inclusion in
 * an ItervalMap
 *
 * The class you use as node within the ItervalMap *must* derive from this class template. I
 * supplies your node class with the necessary members to contain the metainformation for
 * managing the underlying RBTree.
 *
 * See the IntervalMap for more documentation on keys, values, tags and how the IntervalMap behaves.
 *
 * @tparam KeyT			The type of the keys in the IntervalMap.
 * @tparam ValueT		The type of the values in the IntervalMap. Must be default-constructible,
 * addable and subtractable.
 * @tparam Tag			The tag used to identify the underlying RBTree. If you want your nodes to be
 * part of multiple IntervalMaps, RBTrees or IntervalTrees, each must have its own unique tag.
 */
template<class KeyT, class ValueT, int Tag = 0>
class IMapNodeBase
{
public:
	/**
	 * Inserting nodes into an IntervalMap results in the key space to be divided into Segments.
	 * See DOCTODO for details and examples. This is the type that these segments will have.
	 */
	using Segment = internal::InnerNode<KeyT, ValueT, Tag>;

	/// @cond INTERNAL
	Segment _imap_begin;
	Segment _imap_end;

	using value_type = ValueT;
	using key_type = KeyT;
	/// @endcond
};

/**
 * @brief You must derive your own class from this class template, telling the IntervalMap how to
 * interact with your node class.
 *
 * You must derive from this class template and supply the IntervalMap with your own derived
 * class. At the least, you have to implement the methods get_lower, get_upper and get_value for
 * the IntervalMap to work. See the respective methods' documentation for details.
 *
 * @tparam Node 	Your node class to be used in the IntervalMap
 */
template<class Node>
class IMapNodeTraits {
public:
	using key_type = typename Node::key_type;
	using value_type = typename Node::value_type;

	/**
	 * Must be implemented to return the lower bound of the interval represented by n.
	 *
	 * @param n The node whose lower interval bound should be returned.
	 * @return Must return the lower interval bound of n
	 */
	static key_type get_lower(const Node & n) = delete;

	/**
	 * Must be implemented to return the upper bound of the interval represented by n.
	 *
	 * @param n The node whose upper interval bound should be returned.
	 * @return Must return the upper interval bound of n
	 */
	static key_type get_upper(const Node & n) = delete;

	/**
	 * Must be implemented to return the value associated with the interval represented by n.
	 *
	 * @param n The node whose associated value should be returned
	 * @return Must return the value associated with n
	 */
	static value_type get_value(const Node & n) = delete;

	/**
	 * Callback that is called when the aggregate value for a segment changes. See DOCTODO for
	 * information on segments.
	 *
	 * @param seg 		The segment that changed
	 * @param old_val The old aggregate value associated with the segment
	 * @param new_val The new aggregate value associated with the segment
	 */
	static void on_value_changed(typename Node::Segment & seg, const value_type & old_val,
	                             const value_type & new_val) {
		(void)seg;
		(void)old_val;
		(void)new_val;
	}

	/**
	 * Callback that is called when the length of a segment changes. See TODO for how to retrieve
	 * the length of a segment; especially note that there are zero-length segments. See DOCTODO
	 * for information on segments.
	 *
	 * @param seg The segment that changed its length.
	 */
	static void on_length_changed(typename Node::Segment & seg)
	{
		(void) seg;
	}

	/**
	 * Callback that is called when a new segment was created. Note that at the point this callback
	 * is called, the aggregate value associated with the segment is not yet determined. See DOCTODO
	 * for information on segments.
	 *
	 * @param seg The newly inserted segment
	 */
	static void on_segment_inserted(typename Node::Segment & seg)
	{
		(void) seg;
	}

	/**
	 * Callback that is called when a segment is destroyed. See DOCTODO for information on segments.
	 *
	 * @param seg The removed segment
	 */
	static void on_segment_removed(typename Node::Segment & seg)
	{
		(void) seg;
	}
};

/**
 * @brief An IntervalMap stores a collection of intervals that are associated with a value and
 * allows for efficient access to aggregate values
 *
 * An IntervalMap stores a collection of intervals that are associated with a value. Where
 * multiple intervals in the interval map overlap, values are aggregated (e.g., by adding them
 * up). The interval map then allows to efficiently query for the aggregated value at a certain
 * point, to iterate the whole "horizon" of intervals and their respective values, and so on.
 *
 * To this end, the whole "horizon", that is, the space between the smallest lower interval
 * border and the largest upper interval border in the map, is divided into segments. A segment
 * represents a contiuous interval (note: *not* necessarily one of the intervals inserted
 * into the map!) during which the aggregate value does not change. Note that with n intervals
 * inserted into the map, there are at most 2n - 1 such segments. In fact, for implementation
 * reasons, there will always be exactly 2n - 1 segments in an IntervalMap. However, where
 * multiple intervals start or end at the same point, segments of length 0 occurr.
 *
 * @tparam Node					The node class for the interval map. Must be derived from IMapNodeBase.
 * @tparam NodeTraits		The node traits, mainly defining how the IntervalMap retrieves data from
 * your nodes. Must be derived from IMapNodeTraits.
 * @tparam Tag					The tag used to identify the underlying RBTree. If you want your nodes to be
 * part of multiple IntervalMaps, RBTrees or IntervalTrees, each must have its own unique tag.
 */
template <class Node, class NodeTraits, int Tag = 0>
class IntervalMap {
public:
	static_assert(std::is_base_of<IMapNodeTraits<Node>, NodeTraits>::value,
	              "NodeTraits not properly derived from IMapNodeTraits!");

	using NB = IMapNodeBase<typename Node::key_type, typename Node::value_type, Tag>;
	static_assert(std::is_base_of<NB, Node>::value,
	              "Node class not properly derived from IMapNodeBase!");
	using Segment = internal::InnerNode<typename Node::key_type, typename Node::value_type, Tag>;
	using ITree = RBTree<Segment, RBDefaultNodeTraits<Segment>, TreeOptions<TreeFlags::MULTIPLE>,
	                     Tag, typename Segment::Compare>;

	/**
	 * @brief Inserts a node into the IntervalMap.
	 *
	 * @param n The node to be inserted.
	 */
	void insert(Node & n);

	/**
	 * @brief Removes a node from the IntervalMap.
	 *
	 * @param n The node to be removed.
	 */
	void remove(Node & n);

	using value_type = typename Node::value_type;
	using key_type = typename Node::key_type;

	/**
	 * @brief Returns the aggregate value during a segment
	 *
	 * @param s 	The segment the aggregate value of which should be returned
	 * @return 		The aggregate value during s
	 */
	value_type get_aggregate(Segment & s);

	template<class ConcreteIterator, class InnerIterator>
	class IteratorBase {
	public:
		/// @cond INTERNAL


		typedef typename InnerIterator::difference_type      difference_type;
		typedef typename InnerIterator::value_type           value_type;
		typedef typename InnerIterator::reference            reference;
		typedef typename InnerIterator::pointer              pointer;
		typedef std::input_iterator_tag             iterator_category;

		IteratorBase();
		IteratorBase(const InnerIterator & it, ITree * t);
		IteratorBase(const ConcreteIterator & other);

		ConcreteIterator& operator=(const ConcreteIterator & other);
		ConcreteIterator& operator=(ConcreteIterator && other);

		bool operator==(const ConcreteIterator & other) const;
		bool operator!=(const ConcreteIterator & other) const;

		ConcreteIterator& operator++();
		ConcreteIterator  operator++(int);
		ConcreteIterator& operator+=(size_t steps);
		ConcreteIterator  operator+(size_t steps) const;

		ConcreteIterator& operator--();
		ConcreteIterator  operator--(int);

		reference operator*() const;
		pointer operator->() const;
		/// @endcond

		key_type get_lower() const;
		key_type get_upper() const;
		const typename IntervalMap<Node, NodeTraits, Tag>::value_type & get_value() const;

	private:
		/// @cond INTERNAL

		InnerIterator lower;
		InnerIterator upper;
		ITree * t;

		/// @endcond
	};

	class const_iterator : public IteratorBase<const_iterator,
	                                           typename ITree::template const_iterator<false>> {
	public:
		using IteratorBase<const_iterator, typename ITree::template const_iterator<false>>::IteratorBase;
	};

	class iterator : public IteratorBase<iterator, typename ITree::template iterator<false>> {
	public:
		using IteratorBase<iterator, typename ITree::template iterator<false>>::IteratorBase;
	};

	const_iterator begin() const;
	const_iterator end() const;
	iterator begin();
	iterator end();

private:
	ITree t;


};

} // namespace ygg

#include "intervalmap.cpp"

#endif // YGG_INTERVALMAP_HPP

