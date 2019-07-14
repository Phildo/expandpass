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

`-f[aA|A|a|#|aA#|@|l] [#]` Filters output by properties (use --help for details). Optional number as argument, quantifying requirement. `expandpass -f# -fa -fA -fl 10`

`-c # [progress_file]` Sets how often (default: never) to save progress to a progress file ("set Checkpoint"). Will output # passwords before writing progress to a file (also optionally specified; default: "seed.progress"). `expandpass -c 1000000 my_seed.progress`

`-r [checkpoint_file]` Resume from optionally defined (default: seed.progress) progress file. NOTE- a progress file that was created with a different seed file will have unpredictable results. `expandpass -r my_seed.progress`

`--estimate [@600000]` Prints an estimation of number passwords generated from a given seed file, and prediction of how long it will take to enumerate through them at specified output/s (default: 600000). Note: approximates subgroup lengths in processing modifications; subject to error. `expandpass --estimate @7000`

`--unroll #` Specifies cutoff of group size, below which is fit for unrolling (optimizing into a single flat options group) (default 1000; 0 == don't unroll).

`--normalize` Prints normalized/optimized seed file (as used in actual gen).

`--unquoted` Treats otherwise invalid characters as single-character strings.

`--help` Shows simple usage menu. `expandpass --help`

`--version` Shows version. `expandpass --version`


# THE SEED:

The most basic atom of a seed is a **String**, specified with `""`

Seed File:
```
"banana"
```

Output:
```
banana
```

The empty string can be specified as `""`, or an unquoted `-`

Note: To include `"` within a string, escape it with `\"` (example:`"Hello\"World\""` will yield `Hello"World"`). To include `\`, escape it with `\\`.

The next layer up is an **Option Group**, specified by `{}`

Seed File:
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

Next, **Sequence Group**, specified with `<>`

Seed File:
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

Finally, a **Permutation Group**, specified with `()`

Seed File:
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

Then, as a special case, is the **Modifier**, specified with `[]`
Note- a modifier applies to the grouping *before* the modifier. (See MODIFIER section for details w/ syntax)

Seed File:
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

But the real power of this comes from the fact that all of these things can be arbitrarily nested. For example:

Seed File:
```
(
  "Jean"
  < { "M" "m" } "arry" { "M" "m" } "e" >
)
"123"
[-s1 "0123456789"]
{
"!"
"!!"
}
```

Output:
(run it yourself! it's already in `seed.txt`- it should output 496 lines)

NOTE- seed files have a default implicit Sequence Group `<>` specified around it.


# MODIFIERS:

There are 4 types of modifications: **'i'njections**, **'s'ubstitutions**, **'d'eletions**, and **s'm'art substitutions**.
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

NOTE: THIS WILL NOT GUARANTEE TOTAL UNIQUENESS. Deleting the first letter and substituting the second for "A" is identical to deleting the second letter and substituting the first. Combined modifications can thus be redundant.

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

NOTE: new modifications in sequence are delineated by either newlines, or a defined gamut. So, the previous seed could alternately be specified as:

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

NOTE: The - is a guaranteed single-modification (doesn't need further delineation). So you could identically specify `"banana"[-d1]`


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

Note- There is room for performance improvement. But until I have reason, I have no plan to continue working on this aspect.
That said, some ideas/next steps would be:

- Cache supgroup output for blit when iteration occurs elsewhere.
- Parallelize (probably lower-hanging fruit than it sounds! generation of each output already relies only on nicely contained state object)
- Edit password iterations in place, rather than complete reconstruction (would require decent refactor. might render more-difficult parallelization).


# LICENSE:

Currently MIT, but honestly, convince me otherwise if that's a bad idea. The point: free to use, modify, distribute.


# FURTHER INFO

See `todo.txt` for things that need doing.

