expandpass is a simple string-expander. Useful for cracking passwords you kinda-remember.


# HOW IT WORKS:

You define a seed file, and give it to expandpass as an argument (default is `seed.txt`)

`expandpass -i path/to/seed.txt`

It outputs the full expansion of that seed to stdout, or you can define an output file with

`expandpass -o path/to/output.txt`


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
(run it yourself! it's already in `seed.txt`- it should output 497 lines)

NOTE- the entire seed.txt file has a default implicit Sequence Group `<>` specified around it.


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

You can also specify to try all single-character substitutions- but to do so, you must also define a dictionary of substitution candidates:

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

Smart Substitution looks at the character and tries common substitutions (no dictionary definition required!):

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

NOTE: new modifications in sequence are delineated by either newlines, or a defined dictionary. So, the previous seed could alternately be specified as:

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


# FURTHER INFO

See `todo.txt` for things that need doing. One (embarassing) important thing is, right now, you can't specify `"` as part of a password. So I need to build in escaping them (likely with just a \).

