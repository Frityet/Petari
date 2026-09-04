#pragma once

#include <algorithm>
#include <functional>
#include <utility>

// Code built against the Metrowerks Standard Library uses a small legacy
// extension surface that is not named the same way by modern host standard
// libraries. Keep those source-level names available to unmodified clients.
#if !defined(__MWERKS__)
namespace aurora::compat {

template <typename MemberPointer>
struct LegacyMemberFunction {
  MemberPointer member;

  template <typename Object, typename... Arguments>
  constexpr decltype(auto) operator()(Object&& object, Arguments&&... arguments) const {
    return std::invoke(member, std::forward<Object>(object), std::forward<Arguments>(arguments)...);
  }
};

template <typename Function>
struct LegacyUnaryNegate {
  Function function;

  template <typename Argument>
  constexpr bool operator()(Argument&& argument) const {
    return !std::invoke(function, std::forward<Argument>(argument));
  }
};

template <typename Function, typename Value>
struct LegacyBindSecond {
  Function function;
  Value value;

  template <typename Argument>
  constexpr decltype(auto) operator()(Argument&& argument) const {
    return function(std::forward<Argument>(argument), value);
  }
};

template <typename MemberPointer>
struct LegacySecondArgument;

template <typename Result, typename Object, typename Argument>
struct LegacySecondArgument<Result (Object::*)(Argument)> {
  using type = Argument;
};

template <typename Result, typename Object, typename Argument>
struct LegacySecondArgument<Result (Object::*)(Argument) const> {
  using type = Argument;
};

template <typename Result, typename Object, typename Argument>
struct LegacySecondArgument<Result (Object::*)(Argument) noexcept>
    : LegacySecondArgument<Result (Object::*)(Argument)> {};

template <typename Result, typename Object, typename Argument>
struct LegacySecondArgument<Result (Object::*)(Argument) const noexcept>
    : LegacySecondArgument<Result (Object::*)(Argument) const> {};

} // namespace aurora::compat

namespace std {

template <typename MemberPointer>
constexpr auto mem_func(MemberPointer member) {
  return aurora::compat::LegacyMemberFunction<MemberPointer>{member};
}

template <typename MemberPointer>
constexpr auto mem_fun_ref(MemberPointer member) {
  return aurora::compat::LegacyMemberFunction<MemberPointer>{member};
}

// Specialize on our adapter so standard libraries retaining their removed
// negator overload can coexist with clients of the original member adapter.
template <typename MemberPointer>
constexpr auto not1(const aurora::compat::LegacyMemberFunction<MemberPointer>& function) {
  return aurora::compat::LegacyUnaryNegate<aurora::compat::LegacyMemberFunction<MemberPointer>>{function};
}

// Specialize on our adapter so host libraries that still expose their obsolete
// bind2nd overload can coexist with MSL. Store the member's declared argument
// type: value parameters are copied/converted and reference parameters retain
// their actual referent, as in the original binder2nd instantiations.
template <typename MemberPointer, typename Value>
constexpr auto bind2nd(const aurora::compat::LegacyMemberFunction<MemberPointer>& function, const Value& value) {
  using Argument = typename aurora::compat::LegacySecondArgument<MemberPointer>::type;
  return aurora::compat::LegacyBindSecond<aurora::compat::LegacyMemberFunction<MemberPointer>, Argument>{
      function, static_cast<Argument>(value)};
}

template <typename InputIt, typename UnaryPredicate>
InputIt rfind_if(InputIt first, InputIt last, UnaryPredicate predicate) {
  for (; first != last && !predicate(*first); --first) {}

  return first;
}

} // namespace std
#endif
