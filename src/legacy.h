
namespace TupleHelpers {
// template <typename Tuple,
//           typename Indices =
//               std::make_index_sequence<std::tuple_size<Tuple>::value>>
// struct runtime_get_func_table;

// template <typename Tuple, size_t... Indices>
// struct runtime_get_func_table<Tuple, std::index_sequence<Indices...>> {
//   using return_type = typename std::tuple_element<0, Tuple>::type &;
//   using get_func_ptr = return_type (*)(Tuple &) noexcept;
//   static constexpr get_func_ptr table[std::tuple_size<Tuple>::value] = {
//       &std::get<Indices>...};
// };

// template <typename Tuple, size_t... Indices>
// constexpr typename runtime_get_func_table<
//     Tuple, std::index_sequence<Indices...>>::get_func_ptr
//     runtime_get_func_table<Tuple, std::index_sequence<Indices...>>::table
//         [std::tuple_size<Tuple>::value];

// template <typename Tuple>
// constexpr typename std::tuple_element<
//     0, typename std::remove_reference<Tuple>::type>::type &
// runtime_get(Tuple &&t, size_t index) {
//   using tuple_type = typename std::remove_reference<Tuple>::type;
//   // if (index >= std::tuple_size<tuple_type>::value)
//   //   throw std::runtime_error("Out of range");
//   return runtime_get_func_table<tuple_type>::table[index](t);
// }

////////////

// template <typename... T, std::size_t... I>
// auto subtuple_(const std::tuple<T...> &t, std::index_sequence<I...>) {
//   return std::make_tuple(std::get<I>(t)...);
// }

// template <int Trim, typename... T> auto subtuple(const std::tuple<T...> &t) {
//   return subtuple_(t, std::make_index_sequence<sizeof...(T) - Trim>());
// }

///////////////////

// template <typename F, size_t... Is>
// auto gen_tuple_impl(F func, std::index_sequence<Is...>) {
//   return std::make_tuple(func(Is)...);
// }

// template <size_t N, typename F> auto gen_tuple(F func) {
//   return gen_tuple_impl(func, std::make_index_sequence<N>{});
// }

/////////////////////////////

// template <typename Tuple, unsigned n> struct Arg {
//   template <class X, class... Xs> constexpr auto operator()(X x, Xs... xs)
//   {
//     return Arg<n - 1>{}(xs...);
//   }
// };
// template <> struct Arg<0> {
//   template <class X, class... Xs> constexpr auto operator()(X x, Xs...) {
//     return x;
//   }
// };
// template <unsigned n> constexpr auto arg = Arg<n>{};
// arg<2>(0,1,2,3,4,5) == 2;
} // namespace TupleHelpers

// template <typename... Args> std::tuple<Args...> fillTuple(bool &valid)
// const {
//   if (size() != sizeof...(Args)) {
//     valid = false;
//     return {};
//   }
//   valid = true;
//   // size_t i = 0;
//   // for (const auto &v : args) {
//   //   TupleHelpers::runtime_get(t, i) =
//   //       (decltype(TupleHelpers::runtime_get(t, i)))v;
//   //   i++;
//   // }

//   // return TupleHelpers::gen_tuple<sizeof...(Args)>([this](size_t i) {
//   //   return (std::tuple_element<i, std::tuple<Args...>>::type)args[i];
//   // });
//   std::tuple<Args...> t;
//   // fillOneInTuple<std::tuple<Args...>, sizeof...(Args) - 1>(t);
//   TupleFiller::fillOne<sizeof...(Args) - 1>(t, args);
//   return t;
// }

// template <typename Tuple, int I> void fillOneInTuple(Tuple &tuple) const {
//   // constexpr int i = std::tuple_size<Tuple>::value -
//   sizeof...(Remaining); std::get<I>(tuple) =
//       args[I].get<typename std::tuple_element<I, Tuple>::type>();
//   if constexpr (I > 0) {
//     fillOneInTuple<Tuple, I - 1>(tuple);
//   }
// }
