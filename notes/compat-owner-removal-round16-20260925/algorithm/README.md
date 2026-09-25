# Remove nonstandard flat-array adapter

The only four production uses of the injected std::for_each_array helper now use ordinary pointer iteration on their actual Game arrays, matching neighboring recovered code. Each call still visits the same elements in order and invokes the same member with the same arguments. Deleted MetrowerksAlgorithm.hpp and its forced include. No substitute utility or namespace-std extension remains.
