#ifndef GRAPH_CANON_ORDERED_GRAPH_HPP
#define GRAPH_CANON_ORDERED_GRAPH_HPP

#include <boost/graph/graph_concepts.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/type_traits/conditional.hpp>

#include <type_traits>


// An ordered_graph is a view on an underlying graph but where iteration through
// vertices, in-edges, and out-edges are ordered according to a given index map
// over the vertices.
// Note: iteration through the edge list is not supported.
// Note: parallel edges are ordered arbitrarily among each other.

namespace graph_canon {
using boost::graph_traits;
using boost::vertex_list_graph_tag;
using boost::incidence_graph_tag;
using boost::adjacency_graph_tag;
using boost::bidirectional_graph_tag;
using boost::VertexListGraphConcept;
using boost::IncidenceGraphConcept;
using boost::ReadablePropertyMapConcept;
using boost::is_directed_graph;
namespace detail {

template<typename Graph>
struct ordered_adj_incidence {
	typename graph_traits<Graph>::vertex_descriptor v;
	std::vector<typename graph_traits<Graph>::edge_descriptor> out_edges;
};

template<typename Graph>
struct ordered_adj_bidirectional : ordered_adj_incidence<Graph> {
	std::vector<typename graph_traits<Graph>::edge_descriptor> in_edges;
};

template<typename Graph, typename IndexMap, typename Adj = ordered_adj_incidence<Graph> >
struct ordered_incidence {

	struct traversal_category :
	virtual vertex_list_graph_tag,
	virtual incidence_graph_tag,
	virtual adjacency_graph_tag {
	};

	struct vertex_iterator : boost::iterator_facade<vertex_iterator, typename graph_traits<Graph>::vertex_descriptor,
	std::random_access_iterator_tag, typename graph_traits<Graph>::vertex_descriptor> {
		typedef boost::iterator_facade<vertex_iterator, typename graph_traits<Graph>::vertex_descriptor,
		std::random_access_iterator_tag, typename graph_traits<Graph>::vertex_descriptor> base_type;

		vertex_iterator() { }

		explicit vertex_iterator(typename std::vector<Adj>::const_iterator iter) : iter(iter) { }
	private:
		friend class boost::iterator_core_access;

		typename graph_traits<Graph>::vertex_descriptor dereference() const {
			return iter->v;
		}

		void increment() {
			++iter;
		}

		void decrement() {
			--iter;
		}

		bool equal(const vertex_iterator &other) const {
			return iter == other.iter;
		}

		void advance(typename base_type::difference_type n) {
			iter += n;
		}

		typename base_type::difference_type distance_to(const vertex_iterator &other) const {
			return other.iter - iter;
		}
	private:
		typename std::vector<Adj>::const_iterator iter;
	};

	typedef typename std::vector<typename graph_traits<Graph>::edge_descriptor>::const_iterator out_edge_iterator;

	struct adjacency_iterator : boost::iterator_facade<adjacency_iterator, typename graph_traits<Graph>::vertex_descriptor,
	std::random_access_iterator_tag, typename graph_traits<Graph>::vertex_descriptor> {
		typedef boost::iterator_facade<adjacency_iterator, typename graph_traits<Graph>::vertex_descriptor,
		std::random_access_iterator_tag, typename graph_traits<Graph>::vertex_descriptor> base_type;

		adjacency_iterator() { }

		explicit adjacency_iterator(const Graph *g, const out_edge_iterator &iter) : g(g), iter(iter) { }
	private:
		friend class boost::iterator_core_access;

		typename graph_traits<Graph>::vertex_descriptor dereference() const {
			return target(*iter, *g);
		}

		void increment() {
			++iter;
		}

		void decrement() {
			--iter;
		}

		bool equal(const adjacency_iterator &other) const {
			return g == other.g && iter == other.iter;
		}

		void advance(typename base_type::difference_type n) {
			iter += n;
		}

		typename base_type::difference_type distance_to(const adjacency_iterator &other) const {
			return other.iter - iter;
		}
	private:
		const Graph *g;
		out_edge_iterator iter;
	};

	typedef void in_edge_iterator;
	typedef void inv_adjacency_iterator;

	template<typename EdgeLess>
	struct out_edge_less {

		out_edge_less(const Graph &g, IndexMap idx, EdgeLess edge_less) : g(g), idx(idx), edge_less(edge_less) { }

		bool operator()(const typename graph_traits<Graph>::edge_descriptor &lhs,
				const typename graph_traits<Graph>::edge_descriptor &rhs) const {
			typename graph_traits<Graph>::vertices_size_type idx_lhs = idx[target(lhs, g)], idx_rhs = idx[target(rhs, g)];
			if(idx_lhs != idx_rhs) return idx_lhs < idx_rhs;
			else return edge_less(lhs, rhs);
		}
	private:
		const Graph &g;
		IndexMap idx;
		EdgeLess edge_less;
	};
public:

	template<typename EdgeLess>
	ordered_incidence(const Graph &g, IndexMap idx, EdgeLess edge_less) : g(g), idx(idx) {
		data.resize(num_vertices(g));
		out_edge_less<EdgeLess> less(g, idx, edge_less);

		BGL_FORALL_VERTICES_T(v, g, Graph) {
			std::size_t v_idx = get(idx, v);
			Adj &adj = data[v_idx];
			adj.v = v;
			adj.out_edges.reserve(out_degree(v, g));

			BGL_FORALL_OUTEDGES_T(v, e_out, g, Graph) {
				adj.out_edges.push_back(e_out);
			}
			std::sort(adj.out_edges.begin(), adj.out_edges.end(), less);
		}
	}

	//protected:
	const Graph &g;
	IndexMap idx;
	std::vector<Adj> data;
};

template<typename Graph, typename IndexMap, typename Adj = ordered_adj_bidirectional<Graph> >
struct ordered_bidirectional : ordered_incidence<Graph, IndexMap, Adj> {

	struct traversal_category :
	virtual ordered_incidence<Graph, Adj>::traversal_category,
	virtual bidirectional_graph_tag {
	};

	typedef typename std::vector<typename graph_traits<Graph>::edge_descriptor>::const_iterator in_edge_iterator;

	struct inv_adjacency_iterator : boost::iterator_facade<inv_adjacency_iterator, typename graph_traits<Graph>::vertex_descriptor,
	std::random_access_iterator_tag, typename graph_traits<Graph>::vertex_descriptor> {
		typedef boost::iterator_facade<inv_adjacency_iterator, typename graph_traits<Graph>::vertex_descriptor,
		std::random_access_iterator_tag, typename graph_traits<Graph>::vertex_descriptor> base_type;

		inv_adjacency_iterator() { }

		explicit inv_adjacency_iterator(const Graph *g, const in_edge_iterator &iter) : g(g), iter(iter) { }
	private:
		friend class boost::iterator_core_access;

		typename graph_traits<Graph>::vertex_descriptor dereference() const {
			return source(*iter, *g);
		}

		void increment() {
			++iter;
		}

		void decrement() {
			--iter;
		}

		bool equal(const inv_adjacency_iterator &other) const {
			return g == other.g && iter == other.iter;
		}

		void advance(typename base_type::difference_type n) {
			iter += n;
		}

		typename base_type::difference_type distance_to(const inv_adjacency_iterator &other) const {
			return other.iter - iter;
		}
	private:
		const Graph *g;
		in_edge_iterator iter;
	};

	template<typename EdgeLess>
	struct in_edge_less {

		in_edge_less(const Graph &g, IndexMap idx, EdgeLess edge_less) : g(g), idx(idx), edge_less(edge_less) { }

		bool operator()(const typename graph_traits<Graph>::edge_descriptor &lhs,
				const typename graph_traits<Graph>::edge_descriptor &rhs) const {
			typename graph_traits<Graph>::vertices_size_type idx_lhs = idx[source(lhs, g)], idx_rhs = idx[source(rhs, g)];
			if(idx_lhs != idx_rhs) return idx_lhs < idx_rhs;
			else return edge_less(lhs, rhs);
		}
	private:
		const Graph &g;
		IndexMap idx;
		EdgeLess edge_less;
	};
public:

	template<typename EdgeLess>
	ordered_bidirectional(const Graph &g, IndexMap idx, EdgeLess edge_less)
	: ordered_incidence<Graph, IndexMap, Adj>(g, idx, edge_less) {
		in_edge_less<EdgeLess> less(g, idx, edge_less);

		BGL_FORALL_VERTICES_T(v, g, Graph) {
			std::size_t v_idx = get(idx, v);
			Adj &adj = this->data[v_idx];
			adj.in_edges.reserve(in_degree(v, g));

			BGL_FORALL_INEDGES_T(v, e_in, g, Graph) {
				adj.in_edges.push_back(e_in);
			}
			std::sort(adj.in_edges.begin(), adj.in_edges.end(), less);
		}
	}
};

} // namespace detail

// rst: .. class:: template<typename Graph, typename IndexMap, bool WithInEdges = false> \
// rst:            ordered_graph
// rst:
// rst:		A graph adaptor that ensures iteration order in correspondence with a given vertex index map.
// rst:
// rst:		Requires `Graph` to model a `VertexListGraph` and an `IncidenceGraph`,
// rst:		and `IndexMap` a `ReadableProperyMap`.
// rst:		The ordered graph models those concepts as well as `AdjacencyGraph`.
// rst:		If `WithInEdges` is `true`, and `Graph` models a `BidirectionalGraph`,
// rst:		then the ordered graph will model the `BidirectionalGraph` concept as well.
// rst:
// rst:		When iterating through the range of vertices, adjacency vertices, and inverse-adjacent vertices,
// rst:		the vertices will appear in sorted order with respect to the given index map.
// rst:		When iterating through out-edges and in-edges, the edges will appear in sorted order with respect
// rst:		to the index of the neighbouring vertices.
// rst:		If the adapted graph has parallel edges, those will be ordered according to a predicate given to the constructor.
// rst:

template<typename Graph, typename IndexMap, bool WithInEdges = false >
struct ordered_graph {
	BOOST_CONCEPT_ASSERT((VertexListGraphConcept<Graph>));
	BOOST_CONCEPT_ASSERT((IncidenceGraphConcept<Graph>));
	BOOST_STATIC_ASSERT(!WithInEdges || boost::is_bidirectional_graph<Graph>::value);
public:
	using Data = typename boost::conditional<WithInEdges,
			/**/ detail::ordered_bidirectional<Graph, IndexMap>,
			/**/ detail::ordered_incidence<Graph, IndexMap> >::type;
	using Traits = graph_traits<Graph>;
	// Graph
	using vertex_descriptor = typename Traits::vertex_descriptor;
	using edge_descriptor = typename Traits::edge_descriptor;
	using directed_category = typename Traits::directed_category;
	using edge_parallel_category = typename Traits::edge_parallel_category;
	using traversal_category = typename Data::traversal_category;
	// VertexListGraph
	using vertex_iterator = typename Data::vertex_iterator;
	using vertices_size_type = typename Traits::vertices_size_type;
	// IncidenceGraph
	using out_edge_iterator = typename Data::out_edge_iterator;
	using degree_size_type = typename Traits::degree_size_type;
	// AdjacencyGraph
	using adjacency_iterator = typename Data::adjacency_iterator;
	// BidirectionalGraph
	using in_edge_iterator = typename Data::in_edge_iterator; // will be void if not BidirectionalGraph
	// 'AdjacencyBidirectionalGraph'
	using inv_adjacency_iterator = typename Data::inv_adjacency_iterator; // will be void if not BidirectionalGraph
public:

	// rst:		.. function:: template<typename EdgeLess> \
	// rst:		              ordered_graph(const Graph &g, IndexMap idx, EdgeLess edge_less)
	// rst:
	// rst:			Construct an ordered graph from a graph and a given index map (the graph is stored by reference).
	// rst:			The binary edge predicate `edge_less` is used when the graph has parallel edges
	// rst:			(hint: give `always_false()` for simple graphs).

	template<typename EdgeLess>
	ordered_graph(const Graph &g, IndexMap idx, EdgeLess edge_less) : data(g, idx, edge_less) {
		BOOST_CONCEPT_ASSERT((ReadablePropertyMapConcept<IndexMap, typename graph_traits<Graph>::vertex_descriptor>));
	}

	// rst:		.. function:: const Graph &get_graph() const
	// rst:
	// rst:			:returns: a reference to the adapted graph.

	const Graph &get_graph() const {
		return *data.g;
	}

	// rst:		.. function:: IndexMap get_index_map() const
	// rst:
	// rst:			:returns: a copy of the stored index map.

	IndexMap get_index_map() const {
		return data.idx;
	}
	//private:
	Data data;
};

// rst:
// rst: .. function:: template<bool WithInEdges, typename Graph, typename IndexMap, typename EdgeLess> \
// rst:               ordered_graph<Graph, IndexMap, WithInEdges> make_ordered_graph(const Graph &g, IndexMap idx, EdgeLess edge_less)
// rst:
// rst:		:returns: `ordered_graph<Graph, IndexMap, WithInEdges>(g, idx, edge_less)`
// rst:

template<bool WithInEdges, typename Graph, typename IndexMap, typename EdgeLess>
ordered_graph<Graph, IndexMap, WithInEdges> make_ordered_graph(const Graph &g, IndexMap idx, EdgeLess edge_less) {
	return ordered_graph<Graph, IndexMap, WithInEdges>(g, idx, edge_less);
}


// VertexListGraph
//------------------------------------------------------------------------------

template<typename Graph, typename IndexMap, bool WithInEdges>
std::pair<typename graph_traits<ordered_graph<Graph, IndexMap, WithInEdges> >::vertex_iterator,
typename graph_traits<ordered_graph<Graph, IndexMap, WithInEdges> >::vertex_iterator>
vertices(const ordered_graph<Graph, IndexMap, WithInEdges> &g) {
	typedef typename graph_traits<ordered_graph<Graph, IndexMap, WithInEdges> >::vertex_iterator Iter;
	return std::make_pair(Iter(g.data.data.begin()), Iter(g.data.data.end()));
}

template<typename Graph, typename IndexMap, bool WithInEdges>
typename graph_traits<ordered_graph<Graph, IndexMap, WithInEdges> >::vertices_size_type
num_vertices(const ordered_graph<Graph, IndexMap, WithInEdges> &g) {
	return g.data.data.size();
}

// EdgeListGraph (part of it)
//------------------------------------------------------------------------------

template<typename Graph, typename IndexMap, bool WithInEdges>
typename graph_traits<ordered_graph<Graph, IndexMap, WithInEdges> >::vertices_size_type
num_edges(const ordered_graph<Graph, IndexMap, WithInEdges> &g) {
	return num_edges(g.get_graph());
}

// IncidenceGraph
//------------------------------------------------------------------------------

template<typename Graph, typename IndexMap, bool WithInEdges>
std::pair<typename graph_traits<ordered_graph<Graph, IndexMap, WithInEdges> >::out_edge_iterator,
typename graph_traits<ordered_graph<Graph, IndexMap, WithInEdges> >::out_edge_iterator>
out_edges(const typename graph_traits<ordered_graph<Graph, IndexMap, WithInEdges> >::vertex_descriptor &v, const ordered_graph<Graph, IndexMap, WithInEdges> &g) {
	return std::make_pair(
			g.data.data[get(g.data.idx, v)].out_edges.begin(),
			g.data.data[get(g.data.idx, v)].out_edges.end());
}

template<typename Graph, typename IndexMap, bool WithInEdges>
typename graph_traits<ordered_graph<Graph, IndexMap, WithInEdges> >::vertex_descriptor
source(const typename graph_traits<ordered_graph<Graph, IndexMap, WithInEdges> >::edge_descriptor &e, const ordered_graph<Graph, IndexMap, WithInEdges> &g) {
	return source(e, g.data.g);
}

template<typename Graph, typename IndexMap, bool WithInEdges>
typename graph_traits<ordered_graph<Graph, IndexMap, WithInEdges> >::vertex_descriptor
target(const typename graph_traits<ordered_graph<Graph, IndexMap, WithInEdges> >::edge_descriptor &e, const ordered_graph<Graph, IndexMap, WithInEdges> &g) {
	return target(e, g.data.g);
}

template<typename Graph, typename IndexMap, bool WithInEdges>
typename graph_traits<ordered_graph<Graph, IndexMap, WithInEdges> >::degree_size_type
out_degree(const typename graph_traits<ordered_graph<Graph, IndexMap, WithInEdges> >::vertex_descriptor &v, const ordered_graph<Graph, IndexMap, WithInEdges> &g) {
	return g.data.data[get(g.data.idx, v)].out_edges.size();
}

// AdjacencyGraph
//------------------------------------------------------------------------------

template<typename Graph, typename IndexMap, bool WithInEdges>
std::pair<typename graph_traits<ordered_graph<Graph, IndexMap, WithInEdges> >::adjacency_iterator,
typename graph_traits<ordered_graph<Graph, IndexMap, WithInEdges> >::adjacency_iterator>
adjacent_vertices(const typename graph_traits<ordered_graph<Graph, IndexMap, WithInEdges> >::vertex_descriptor &v, const ordered_graph<Graph, IndexMap, WithInEdges> &g) {
	typedef typename graph_traits<ordered_graph<Graph, IndexMap, WithInEdges> >::adjacency_iterator Iter;
	return std::make_pair(
			Iter(&g.data.g, g.data.data[get(g.data.idx, v)].out_edges.begin()),
			Iter(&g.data.g, g.data.data[get(g.data.idx, v)].out_edges.end()));
}

// BidirectionalGraph
//------------------------------------------------------------------------------

template<typename Graph, typename IndexMap>
std::pair<typename graph_traits<ordered_graph<Graph, IndexMap, true> >::in_edge_iterator,
typename graph_traits<ordered_graph<Graph, IndexMap, true> >::in_edge_iterator>
in_edges(const typename graph_traits<ordered_graph<Graph, IndexMap, true> >::vertex_descriptor &v, const ordered_graph<Graph, IndexMap, true> &g) {
	return std::make_pair(
			g.data.data[get(g.data.idx, v)].in_edges.begin(),
			g.data.data[get(g.data.idx, v)].in_edges.end());
}

template<typename Graph, typename IndexMap>
typename graph_traits<ordered_graph<Graph, IndexMap, true> >::degree_size_type
in_degree(const typename graph_traits<ordered_graph<Graph, IndexMap, true> >::vertex_descriptor &v, const ordered_graph<Graph, IndexMap, true> &g) {
	return g.data.data[get(g.data.idx, v)].in_edges.size();
}

template<typename Graph, typename IndexMap>
typename std::enable_if<is_directed_graph<Graph>::value,
/**/ typename graph_traits<ordered_graph<Graph, IndexMap, true> >::degree_size_type
>::type
degree(const typename graph_traits<ordered_graph<Graph, IndexMap, true> >::vertex_descriptor &v, const ordered_graph<Graph, IndexMap, true> &g) {
	return out_degree(v, g) + in_degree(v, g);
}

template<typename Graph, typename IndexMap>
typename std::enable_if<!is_directed_graph<Graph>::value,
/**/ typename graph_traits<ordered_graph<Graph, IndexMap, true> >::degree_size_type
>::type
degree(const typename graph_traits<ordered_graph<Graph, IndexMap, true> >::vertex_descriptor &v, const ordered_graph<Graph, IndexMap, true> &g) {
	return out_degree(v, g);
}


// 'AdjacencyBidirectionalGraph'
//------------------------------------------------------------------------------

template<typename Graph, typename IndexMap>
std::pair<typename /*graph_traits<*/ordered_graph<Graph, IndexMap, true>/* >*/::inv_adjacency_iterator,
typename /*graph_traits<*/ordered_graph<Graph, IndexMap, true>/* >*/::inv_adjacency_iterator>
inv_adjacent_vertices(const typename graph_traits<ordered_graph<Graph, IndexMap, true> >::vertex_descriptor &v, const ordered_graph<Graph, IndexMap, true> &g) {
	typedef typename /*graph_traits<*/ordered_graph<Graph, IndexMap, true>/* >*/::inv_adjacency_iterator Iter;
	return std::make_pair(
			Iter(&g.data.g, g.data.data[get(g.data.idx, v)].in_edges.begin()),
			Iter(&g.data.g, g.data.data[get(g.data.idx, v)].in_edges.end()));
}

} // namespace graph_canon

#endif /* GRAPH_CANONICALIZATION_ORDERED_GRAPH_HPP */