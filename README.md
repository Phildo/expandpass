expandpass is a simple string-expander. Useful for cracking passwords you kinda-remember.


# QUICK EXAMPLE:

Converts a seed file constructed like this:

```
{ "apple" "banana" }( "!" "123" )
```

to a list of expanded strings, like this:

```
apple!123
banana!123
apple123!
banana123!
```

Note: This had to be a (very) short example- because the output grows very fast!


# HOW TO USE:

You define a seed file, and give it to expandpass as an argument (default is `seed.txt`)

`expandpass -i path/to/seed.txt`

It outputs the full expansion of that seed to stdout, or you can define an output file with

`expandpass -o path/to/output.txt`

It can also be run (and behaves as expected) in standard unix-y way

`echo '{ "apple" "banana" }[m5]' | expandpass | grep 4`


# ARGUMENTS:

`-i input_file` Specifies file to use as seed (default seed.txt, also accepts stdin w/o specification)

`-o output_file` Specifies file to print results (default stdout)

`-b #` Specifies buffer size (bytes) to fill before printing to output (default 1M, experimentally doesn't really alter perf as long as its bigger than ~20 bytes)

`-f[aA|A|a|#|aA#|@|lmin|lmax] [#]` Filters output by properties (use --help for details). Optional number as argument, quantifying requirement. `expandpass -f# -fa -fA -flmin 10`

`-c # [progress_file]` Sets how often (default: never) to save progress to a progress file ("set Checkpoint"). Will output # passwords before writing progress to a file (also optionally specified; default: "seed.progress"). `expandpass -c 1000000 my_seed.progress`

`-r [checkpoint_file]` Resume from optionally defined (default: seed.progress) progress file. Note: a progress file that was created with a different seed file will have unpredictable results. `expandpass -r my_seed.progress`

`--estimate [@600000]` Prints an estimation of number passwords generated from a given seed file, and prediction of how long it will take to enumerate through them at specified output/s (default: 600000). Note: approximates subgroup lengths in processing modifications; subject to error. `expandpass --estimate @7000`

`--unroll #` Specifies cutoff of group size, below which is fit for unrolling (optimizing into a single flat options group) (default 1000; 0 == don't unroll).

`--normalize` Prints normalized/optimized seed file (as used in actual gen).

`--unquoted` Treats otherwise invalid characters as single-character strings.

`--help` Shows simple usage menu. `expandpass --help`

`--version` Shows version. `expandpass --version`


# COMPILE:

expandpass is written using C++, but only uses the c standard library (conversion to pure C should be relatively simple, if necessary).
`g++ gen.cpp -o expandpass` (or similar w/ any C++ compiler) should be sufficient.
There is a Makefile and CMakeLists for the purposes of stubbing out a basic build system,
but they were built with only my simple environment in mind.


# THE SEED:

The "seed" is the nested input to expandpass, describing the desired expansion.

---------------------

**"String"**: The most basic atom of a seed, specified with `""`

Seed:
```
"banana"
```

Output:
```
banana
```

The empty string can be specified as `""`, or an unquoted `-`

Note: To include `"` within a string, escape it with `\"` (example:`"Hello\"World\""` will yield `Hello"World"`). To include `\`, escape it with `\\`.

---------------------

**{Option Group}**: Choose a member, specified by `{}`

Seed:
```
{
"banana"
"apple"
}
```

Output:
```
banana
apple
```

---------------------

**<Sequence Group>**: Concatenate members, specified with `<>`

Seed:
```
<
"banana"
"apple"
>
```

Output:
```
bananaapple
```

---------------------

**(Permutation Group)**: Shuffle member order, specified with `()`

Seed:
```
(
"banana"
"apple"
)
```

Output:
```
bananaapple
applebanana
```

---------------------

**[Modifier]**\*: Modify the *previous* group, specified with `[]`

\*Modifiers are not "groups" themselves, but rather contain instructions to modify the previous group. (See MODIFIER section for details w/ syntax)

Seed:
```
"banana"
[ s1 "abcdefghijklmnopqrstuvwxyz" ]
```

Output:
```
aanana
banana
canana
...
zanana
banana
bbnana
bcnana
...
bananz
```

---------------------

**Put it together**:

The real power of the above groupings comes from the fact that all of these things can be arbitrarily nested. For example:

Seed:
```
(
  "Jean"
  < { "M" "m" } "arry" { "M" "m" } "e" >
)
"123"
[-s1 "0123456789"]
{
-
"!"
"!!"
}
```

Output:

(run it yourself! it's already in `seed.txt`- it should output 744 lines)

Note: seeds are implicitly are wrapped by a global Sequence Group (`<...>`).

Note: you can leave comments with an unquoted `#` character. Everything proceeding the `#` on that line will be ignored.

# MODIFIERS:

There are 5 types of modifications: **'i'njections**, **'s'ubstitutions**, **'d'eletions**, **s'm'art substitutions**, and **'c'opies**
For the sake of simplicity, I'll assume each modification needs to be on its own line (though that isn't syntactually enforced)

You specify that you want to modify the previously specified group (or string) with `[]`, and you specify what the modification should be within.
For example, if I wanted all instances of "banana" but with one character deleted, I would put in my seed file:

```
"banana"
[
d1
]
```

That means, "try all single deletions on this string", yielding:

```
"anana"
"bnana"
"baana"
"banna"
"banaa"
"banan"
```

You can also specify to try all single-character substitutions- but to do so, you must also define a gamut of substitution candidates:

```
"banana"
[
s1 "ABC"
]
```

which yields:

```
"Aanana"
"Banana"
"Canana"
"bAnana"
...
"bananC"
```

Injection is similarly understood:

```
"banana"
[
i1 "ABC"
]
```

yeilds:
```
"Abanana"
"Bbanana"
"Cbanana"
"bAanana"
...
"bananaC"
```

Smart Substitution looks at the character and tries common substitutions (no gamut definition required!):

```
"banana"
[
m1
]

"Banana"
"bAnana"
"b4nana"
"baNana"
...
```

Copy just repeats the previous group n times

```
"banana"
[
c3
]

"bananabananabanana"
```

And again, the power in these comes from their composability. The simplest one is to increment the 1 to a 2

```
"banana"
[
s2 "ABC"
]
```

yields:

```
"AAnana"
"ABnana"
...
"banaCC"
```

Multiple modification types can be composed:

```
"banana"
[
s1d1 "ABC"
]
```

```
"Anana"
"Bnana"
"Cnana"
"bAana"
"bBana"
"bCana"
...
```

Note: THIS WILL NOT GUARANTEE TOTAL UNIQUENESS. Deleting the first letter and substituting the second for "A" is identical to deleting the second letter and substituting the first. Combined modifications can thus be redundant.

You can compose modifications in sequence as well:

```
"banana"
[
  d1
  s1 "ABC"
]
```

yields:

```
"anana"
... (all the deletes)
"banan"
"Aanana"
"Banana"
..
```

Note: new modifications in sequence are delineated by either newlines, or a defined gamut. So, the previous seed could alternately be specified as:

```
"banana" [ d1 "" s1 "ABC" ]
```

Finally, the "null" modification is the dash -

```
"banana"
[
-
d1
]
```

will ensure `banana` is printed unmodified once before going into the modifications.

Note: The - is a guaranteed single-modification (doesn't need further delineation). So you could identically specify `"banana"[-d1]`


# PERFORMANCE:

Benchmarked with:

```
time echo '( "Jean" < { "M" "m" } "arry" { "M" "m" } "e" > ) [m5] "123" [-s1 "0123456789"] { "!" "!!" }' | expandpass | wc -l
```

with worst-case output (over a handful of trials):

```
  954800

real	0m0.507s
user	0m0.500s
sys	0m0.020s
```

on a 2014 Macbook Air, 1.4 GHz Intel Core i5. This comes out to ~2M lines/s.

Note: There is room for performance improvement. But until I have reason, I have no plan to continue working on this aspect.
That said, some ideas/next steps would be:

- Cache supgroup output for blit when iteration occurs elsewhere.
- Parallelize (probably lower-hanging fruit than it sounds! generation of each output already relies only on nicely contained state object)
- Edit password iterations in place, rather than complete reconstruction (would require decent refactor. might render more-difficult parallelization).


# LICENSE:

MIT


# FURTHER INFO

See `todo.txt` for things that need doing.

