/**********************************************************************

  struct.c -

  $Author: usa $
  created at: Tue Mar 22 18:44:30 JST 1995

  Copyright (C) 1993-2007 Yukihiro Matsumoto

**********************************************************************/

#include "ruby/ruby.h"
#include "internal.h"

VALUE rb_cStruct;
static ID id_members;

static VALUE struct_alloc(VALUE);

static inline VALUE
struct_ivar_get(VALUE c, ID id)
{
    for (;;) {
	if (rb_ivar_defined(c, id))
	    return rb_ivar_get(c, id);
	c = RCLASS_SUPER(c);
	if (c == 0 || c == rb_cStruct)
	    return Qnil;
    }
}

VALUE
rb_struct_iv_get(VALUE c, const char *name)
{
    return struct_ivar_get(c, rb_intern(name));
}

VALUE
rb_struct_s_members(VALUE klass)
{
    VALUE members = struct_ivar_get(klass, id_members);

    if (NIL_P(members)) {
	rb_raise(rb_eTypeError, "uninitialized struct");
    }
    if (!RB_TYPE_P(members, T_ARRAY)) {
	rb_raise(rb_eTypeError, "corrupted struct");
    }
    return members;
}

VALUE
rb_struct_members(VALUE s)
{
    VALUE members = rb_struct_s_members(rb_obj_class(s));

    if (RSTRUCT_LEN(s) != RARRAY_LEN(members)) {
	rb_raise(rb_eTypeError, "struct size differs (%ld required %ld given)",
		 RARRAY_LEN(members), RSTRUCT_LEN(s));
    }
    return members;
}

static VALUE
rb_struct_s_members_m(VALUE klass)
{
    VALUE members = rb_struct_s_members(klass);

    return rb_ary_dup(members);
}

/*
 *  call-seq:
 *     struct.members    -> array
 *
 *  Returns an array of symbols representing the names of the instance
 *  variables.
 *
 *     Customer = Struct.new(:name, :address, :zip)
 *     joe = Customer.new("Joe Smith", "123 Maple, Anytown NC", 12345)
 *     joe.members   #=> [:name, :address, :zip]
 */

static VALUE
rb_struct_members_m(VALUE obj)
{
    return rb_struct_s_members_m(rb_obj_class(obj));
}

NORETURN(static void not_a_member(ID id));
static void
not_a_member(ID id)
{
    rb_name_error(id, "`%"PRIsVALUE"' is not a struct member", QUOTE_ID(id));
}

VALUE
rb_struct_getmember(VALUE obj, ID id)
{
    VALUE members, slot, *ptr, *ptr_members;
    long i, len;

    ptr = RSTRUCT_PTR(obj);
    members = rb_struct_members(obj);
    ptr_members = RARRAY_PTR(members);
    slot = ID2SYM(id);
    len = RARRAY_LEN(members);
    for (i=0; i<len; i++) {
	if (ptr_members[i] == slot) {
	    return ptr[i];
	}
    }
    not_a_member(id);

    UNREACHABLE;
}

static VALUE
rb_struct_ref(VALUE obj)
{
    return rb_struct_getmember(obj, rb_frame_this_func());
}

static VALUE rb_struct_ref0(VALUE obj) {return RSTRUCT_PTR(obj)[0];}
static VALUE rb_struct_ref1(VALUE obj) {return RSTRUCT_PTR(obj)[1];}
static VALUE rb_struct_ref2(VALUE obj) {return RSTRUCT_PTR(obj)[2];}
static VALUE rb_struct_ref3(VALUE obj) {return RSTRUCT_PTR(obj)[3];}
static VALUE rb_struct_ref4(VALUE obj) {return RSTRUCT_PTR(obj)[4];}
static VALUE rb_struct_ref5(VALUE obj) {return RSTRUCT_PTR(obj)[5];}
static VALUE rb_struct_ref6(VALUE obj) {return RSTRUCT_PTR(obj)[6];}
static VALUE rb_struct_ref7(VALUE obj) {return RSTRUCT_PTR(obj)[7];}
static VALUE rb_struct_ref8(VALUE obj) {return RSTRUCT_PTR(obj)[8];}
static VALUE rb_struct_ref9(VALUE obj) {return RSTRUCT_PTR(obj)[9];}

#define numberof(array) (int)(sizeof(array) / sizeof((array)[0]))
#define N_REF_FUNC numberof(ref_func)

static VALUE (*const ref_func[])(VALUE) = {
    rb_struct_ref0,
    rb_struct_ref1,
    rb_struct_ref2,
    rb_struct_ref3,
    rb_struct_ref4,
    rb_struct_ref5,
    rb_struct_ref6,
    rb_struct_ref7,
    rb_struct_ref8,
    rb_struct_ref9,
};

static void
rb_struct_modify(VALUE s)
{
    rb_check_frozen(s);
    rb_check_trusted(s);
}

static VALUE
rb_struct_set(VALUE obj, VALUE val)
{
    VALUE members, slot, *ptr, *ptr_members;
    long i, len;
    ID fid = rb_frame_this_func();

    members = rb_struct_members(obj);
    ptr_members = RARRAY_PTR(members);
    len = RARRAY_LEN(members);
    rb_struct_modify(obj);
    ptr = RSTRUCT_PTR(obj);
    for (i=0; i<len; i++) {
	slot = ptr_members[i];
	if (rb_id_attrset(SYM2ID(slot)) == fid) {
	    return ptr[i] = val;
	}
    }
    not_a_member(fid);

    UNREACHABLE;
}

static VALUE
anonymous_struct(VALUE klass)
{
    VALUE nstr;

    nstr = rb_class_new(klass);
    rb_make_metaclass(nstr, RBASIC(klass)->klass);
    rb_class_inherited(klass, nstr);
    return nstr;
}

static VALUE
new_struct(VALUE name, VALUE super)
{
    /* old style: should we warn? */
    ID id;
    name = rb_str_to_str(name);
    if (!rb_is_const_name(name)) {
	rb_name_error_str(name, "identifier %"PRIsVALUE" needs to be constant",
			  QUOTE(name));
    }
    id = rb_to_id(name);
    if (rb_const_defined_at(super, id)) {
	rb_warn("redefining constant %"PRIsVALUE"::%"PRIsVALUE, super, name);
	rb_mod_remove_const(super, ID2SYM(id));
    }
    return rb_define_class_id_under(super, id, super);
}

static VALUE
setup_struct(VALUE nstr, VALUE members)
{
    VALUE *ptr_members;
    long i, len;

    OBJ_FREEZE(members);
    rb_ivar_set(nstr, id_members, members);

    rb_define_alloc_func(nstr, struct_alloc);
    rb_define_singleton_method(nstr, "new", rb_class_new_instance, -1);
    rb_define_singleton_method(nstr, "[]", rb_class_new_instance, -1);
    rb_define_singleton_method(nstr, "members", rb_struct_s_members_m, 0);
    ptr_members = RARRAY_PTR(members);
    len = RARRAY_LEN(members);
    for (i=0; i< len; i++) {
	ID id = SYM2ID(ptr_members[i]);
	if (i < N_REF_FUNC) {
	    rb_define_method_id(nstr, id, ref_func[i], 0);
	}
	else {
	    rb_define_method_id(nstr, id, rb_struct_ref, 0);
	}
	rb_define_method_id(nstr, rb_id_attrset(id), rb_struct_set, 1);
    }

    return nstr;
}

VALUE
rb_struct_alloc_noinit(VALUE klass)
{
    return struct_alloc(klass);
}

VALUE
rb_struct_define_without_accessor(const char *class_name, VALUE super, rb_alloc_func_t alloc, ...)
{
    VALUE klass;
    va_list ar;
    VALUE members;
    char *name;

    members = rb_ary_tmp_new(0);
    va_start(ar, alloc);
    while ((name = va_arg(ar, char*)) != NULL) {
        rb_ary_push(members, ID2SYM(rb_intern(name)));
    }
    va_end(ar);
    OBJ_FREEZE(members);

    if (class_name) {
        klass = rb_define_class(class_name, super);
    }
    else {
	klass = anonymous_struct(super);
    }

    rb_ivar_set(klass, id_members, members);

    if (alloc)
        rb_define_alloc_func(klass, alloc);
    else
        rb_define_alloc_func(klass, struct_alloc);

    return klass;
}

VALUE
rb_struct_define(const char *name, ...)
{
    va_list ar;
    VALUE st, ary;
    char *mem;

    ary = rb_ary_tmp_new(0);

    va_start(ar, name);
    while ((mem = va_arg(ar, char*)) != 0) {
	ID slot = rb_intern(mem);
	rb_ary_push(ary, ID2SYM(slot));
    }
    va_end(ar);

    if (!name) st = anonymous_struct(rb_cStruct);
    else st = new_struct(rb_str_new2(name), rb_cStruct);
    return setup_struct(st, ary);
}

/*
 *  call-seq:
 *     Struct.new( [aString] [, aSym]+> )                         -> StructClass
 *     Struct.new( [aString] [, aSym]+> ) {|StructClass| block }  -> StructClass
 *     StructClass.new(arg, ...)                                  -> obj
 *     StructClass[arg, ...]                                      -> obj
 *
 *  Creates a new class, named by <i>aString</i>, containing accessor
 *  methods for the given symbols. If the name <i>aString</i> is
 *  omitted, an anonymous structure class will be created. Otherwise,
 *  the name of this struct will appear as a constant in class
 *  <code>Struct</code>, so it must be unique for all
 *  <code>Struct</code>s in the system and should start with a capital
 *  letter. Assigning a structure class to a constant effectively gives
 *  the class the name of the constant.
 *
 *  If a block is given, it will be evaluated in the context of
 *  <i>StructClass</i>, passing <i>StructClass</i> as a parameter.
 *
 *     Customer = Struct.new(:name, :address) do
 *       def greeting
 *         "Hello #{name}!"
 *       end
 *     end
 *     Customer.new("Dave", "123 Main").greeting  # => "Hello Dave!"
 *
 *  <code>Struct::new</code> returns a new <code>Class</code> object,
 *  which can then be used to create specific instances of the new
 *  structure. The number of actual parameters must be
 *  less than or equal to the number of attributes defined for this
 *  class; unset parameters default to <code>nil</code>.  Passing too many
 *  parameters will raise an <code>ArgumentError</code>.
 *
 *  The remaining methods listed in this section (class and instance)
 *  are defined for this generated class.
 *
 *     # Create a structure with a name in Struct
 *     Struct.new("Customer", :name, :address)    #=> Struct::Customer
 *     Struct::Customer.new("Dave", "123 Main")   #=> #<struct Struct::Customer name="Dave", address="123 Main">
 *
 *     # Create a structure named by its constant
 *     Customer = Struct.new(:name, :address)     #=> Customer
 *     Customer.new("Dave", "123 Main")           #=> #<struct Customer name="Dave", address="123 Main">
 */

static VALUE
rb_struct_s_def(int argc, VALUE *argv, VALUE klass)
{
    VALUE name, rest;
    long i;
    VALUE st;
    ID id;

    rb_check_arity(argc, 1, UNLIMITED_ARGUMENTS);
    name = argv[0];
    if (SYMBOL_P(name)) {
	name = Qnil;
    }
    else {
	--argc;
	++argv;
    }
    rest = rb_ary_tmp_new(argc);
    for (i=0; i<argc; i++) {
	id = rb_to_id(argv[i]);
	RARRAY_PTR(rest)[i] = ID2SYM(id);
	rb_ary_set_len(rest, i+1);
    }
    if (NIL_P(name)) {
	st = anonymous_struct(klass);
    }
    else {
	st = new_struct(name, klass);
    }
    setup_struct(st, rest);
    if (rb_block_given_p()) {
	rb_mod_module_eval(0, 0, st);
    }

    return st;
}

static long
num_members(VALUE klass)
{
    VALUE members;
    members = struct_ivar_get(klass, id_members);
    if (!RB_TYPE_P(members, T_ARRAY)) {
	rb_raise(rb_eTypeError, "broken members");
    }
    return RARRAY_LEN(members);
}

/*
 */

static VALUE
rb_struct_initialize_m(int argc, VALUE *argv, VALUE self)
{
    VALUE klass = rb_obj_class(self);
    long n;

    rb_struct_modify(self);
    n = num_members(klass);
    if (n < argc) {
	rb_raise(rb_eArgError, "struct size differs");
    }
    MEMCPY(RSTRUCT_PTR(self), argv, VALUE, argc);
    if (n > argc) {
	rb_mem_clear(RSTRUCT_PTR(self)+argc, n-argc);
    }
    return Qnil;
}

VALUE
rb_struct_initialize(VALUE self, VALUE values)
{
    return rb_struct_initialize_m(RARRAY_LENINT(values), RARRAY_PTR(values), self);
}

static VALUE
struct_alloc(VALUE klass)
{
    long n;
    NEWOBJ_OF(st, struct RStruct, klass, T_STRUCT);

    n = num_members(klass);

    if (0 < n && n <= RSTRUCT_EMBED_LEN_MAX) {
        RBASIC(st)->flags &= ~RSTRUCT_EMBED_LEN_MASK;
        RBASIC(st)->flags |= n << RSTRUCT_EMBED_LEN_SHIFT;
	rb_mem_clear(st->as.ary, n);
    }
    else {
	st->as.heap.ptr = ALLOC_N(VALUE, n);
	rb_mem_clear(st->as.heap.ptr, n);
	st->as.heap.len = n;
    }

    return (VALUE)st;
}

VALUE
rb_struct_alloc(VALUE klass, VALUE values)
{
    return rb_class_new_instance(RARRAY_LENINT(values), RARRAY_PTR(values), klass);
}

VALUE
rb_struct_new(VALUE klass, ...)
{
    VALUE tmpargs[N_REF_FUNC], *mem = tmpargs;
    int size, i;
    va_list args;

    size = rb_long2int(num_members(klass));
    if (size > numberof(tmpargs)) {
	tmpargs[0] = rb_ary_tmp_new(size);
	mem = RARRAY_PTR(tmpargs[0]);
    }
    va_start(args, klass);
    for (i=0; i<size; i++) {
	mem[i] = va_arg(args, VALUE);
    }
    va_end(args);

    return rb_class_new_instance(size, mem, klass);
}

static VALUE
rb_struct_size(VALUE s);

/*
 *  call-seq:
 *     struct.each {|obj| block }  -> struct
 *     struct.each                 -> an_enumerator
 *
 *  Calls <i>block</i> once for each instance variable, passing the
 *  value as a parameter.
 *
 *  If no block is given, an enumerator is returned instead.
 *
 *     Customer = Struct.new(:name, :address, :zip)
 *     joe = Customer.new("Joe Smith", "123 Maple, Anytown NC", 12345)
 *     joe.each {|x| puts(x) }
 *
 *  <em>produces:</em>
 *
 *     Joe Smith
 *     123 Maple, Anytown NC
 *     12345
 */

static VALUE
rb_struct_each(VALUE s)
{
    long i;

    RETURN_SIZED_ENUMERATOR(s, 0, 0, rb_struct_size);
    for (i=0; i<RSTRUCT_LEN(s); i++) {
	rb_yield(RSTRUCT_PTR(s)[i]);
    }
    return s;
}

/*
 *  call-seq:
 *     struct.each_pair {|sym, obj| block }     -> struct
 *     struct.each_pair                         -> an_enumerator
 *
 *  Calls <i>block</i> once for each instance variable, passing the name
 *  (as a symbol) and the value as parameters.
 *
 *  If no block is given, an enumerator is returned instead.
 *
 *     Customer = Struct.new(:name, :address, :zip)
 *     joe = Customer.new("Joe Smith", "123 Maple, Anytown NC", 12345)
 *     joe.each_pair {|name, value| puts("#{name} => #{value}") }
 *
 *  <em>produces:</em>
 *
 *     name => Joe Smith
 *     address => 123 Maple, Anytown NC
 *     zip => 12345
 */

static VALUE
rb_struct_each_pair(VALUE s)
{
    VALUE members;
    long i;

    RETURN_SIZED_ENUMERATOR(s, 0, 0, rb_struct_size);
    members = rb_struct_members(s);
    for (i=0; i<RSTRUCT_LEN(s); i++) {
	VALUE key = rb_ary_entry(members, i);
	VALUE value = RSTRUCT_PTR(s)[i];
	rb_yield(rb_assoc_new(key, value));
    }
    return s;
}

static VALUE
inspect_struct(VALUE s, VALUE dummy, int recur)
{
    VALUE cname = rb_class_name(rb_obj_class(s));
    VALUE members, str = rb_str_new2("#<struct ");
    VALUE *ptr, *ptr_members;
    long i, len;
    char first = RSTRING_PTR(cname)[0];

    if (recur || first != '#') {
	rb_str_append(str, cname);
    }
    if (recur) {
	return rb_str_cat2(str, ":...>");
    }

    members = rb_struct_members(s);
    ptr_members = RARRAY_PTR(members);
    ptr = RSTRUCT_PTR(s);
    len = RSTRUCT_LEN(s);
    for (i=0; i<len; i++) {
	VALUE slot;
	ID id;

	if (i > 0) {
	    rb_str_cat2(str, ", ");
	}
	else if (first != '#') {
	    rb_str_cat2(str, " ");
	}
	slot = ptr_members[i];
	id = SYM2ID(slot);
	if (rb_is_local_id(id) || rb_is_const_id(id)) {
	    rb_str_append(str, rb_id2str(id));
	}
	else {
	    rb_str_append(str, rb_inspect(slot));
	}
	rb_str_cat2(str, "=");
	rb_str_append(str, rb_inspect(ptr[i]));
    }
    rb_str_cat2(str, ">");
    OBJ_INFECT(str, s);

    return str;
}

/*
 * call-seq:
 *   struct.to_s      -> string
 *   struct.inspect   -> string
 *
 * Describe the contents of this struct in a string.
 */

static VALUE
rb_struct_inspect(VALUE s)
{
    return rb_exec_recursive(inspect_struct, s, 0);
}

/*
 *  call-seq:
 *     struct.to_a     -> array
 *     struct.values   -> array
 *
 *  Returns the values for this instance as an array.
 *
 *     Customer = Struct.new(:name, :address, :zip)
 *     joe = Customer.new("Joe Smith", "123 Maple, Anytown NC", 12345)
 *     joe.to_a[1]   #=> "123 Maple, Anytown NC"
 */

static VALUE
rb_struct_to_a(VALUE s)
{
    return rb_ary_new4(RSTRUCT_LEN(s), RSTRUCT_PTR(s));
}

/*
 *  call-seq:
 *     struct.to_h     -> hash
 *
 *  Returns the values for this instance as a hash with keys
 *  corresponding to the instance variable name.
 *
 *     Customer = Struct.new(:name, :address, :zip)
 *     joe = Customer.new("Joe Smith", "123 Maple, Anytown NC", 12345)
 *     joe.to_h[:address]   #=> "123 Maple, Anytown NC"
 */

static VALUE
rb_struct_to_h(VALUE s)
{
    VALUE h = rb_hash_new();
    VALUE members = rb_struct_members(s);
    long i;

    for (i=0; i<RSTRUCT_LEN(s); i++) {
	rb_hash_aset(h, rb_ary_entry(members, i), RSTRUCT_PTR(s)[i]);
    }
    return h;
}

/* :nodoc: */
VALUE
rb_struct_init_copy(VALUE copy, VALUE s)
{
    if (!OBJ_INIT_COPY(copy, s)) return copy;
    if (RSTRUCT_LEN(copy) != RSTRUCT_LEN(s)) {
	rb_raise(rb_eTypeError, "struct size mismatch");
    }
    MEMCPY(RSTRUCT_PTR(copy), RSTRUCT_PTR(s), VALUE, RSTRUCT_LEN(copy));

    return copy;
}

static VALUE
rb_struct_aref_id(VALUE s, ID id)
{
    VALUE *ptr, members, *ptr_members;
    long i, len;

    ptr = RSTRUCT_PTR(s);
    members = rb_struct_members(s);
    ptr_members = RARRAY_PTR(members);
    len = RARRAY_LEN(members);
    for (i=0; i<len; i++) {
	if (SYM2ID(ptr_members[i]) == id) {
	    return ptr[i];
	}
    }
    rb_name_error(id, "no member '%s' in struct", rb_id2name(id));

    UNREACHABLE;
}

/*
 *  call-seq:
 *     struct[symbol]    -> anObject
 *     struct[fixnum]    -> anObject
 *
 *  Attribute Reference---Returns the value of the instance variable
 *  named by <i>symbol</i>, or indexed (0..length-1) by
 *  <i>fixnum</i>. Will raise <code>NameError</code> if the named
 *  variable does not exist, or <code>IndexError</code> if the index is
 *  out of range.
 *
 *     Customer = Struct.new(:name, :address, :zip)
 *     joe = Customer.new("Joe Smith", "123 Maple, Anytown NC", 12345)
 *
 *     joe["name"]   #=> "Joe Smith"
 *     joe[:name]    #=> "Joe Smith"
 *     joe[0]        #=> "Joe Smith"
 */

VALUE
rb_struct_aref(VALUE s, VALUE idx)
{
    long i;

    if (RB_TYPE_P(idx, T_SYMBOL)) {
	return rb_struct_aref_id(s, SYM2ID(idx));
    }
    else if (RB_TYPE_P(idx, T_STRING)) {
	ID id = rb_check_id(&idx);
	if (!id) {
	    rb_name_error_str(idx, "no member '%"PRIsVALUE"' in struct",
			      QUOTE(idx));
	}
	return rb_struct_aref_id(s, id);
    }

    i = NUM2LONG(idx);
    if (i < 0) i = RSTRUCT_LEN(s) + i;
    if (i < 0)
        rb_raise(rb_eIndexError, "offset %ld too small for struct(size:%ld)",
		 i, RSTRUCT_LEN(s));
    if (RSTRUCT_LEN(s) <= i)
        rb_raise(rb_eIndexError, "offset %ld too large for struct(size:%ld)",
		 i, RSTRUCT_LEN(s));
    return RSTRUCT_PTR(s)[i];
}

static VALUE
rb_struct_aset_id(VALUE s, ID id, VALUE val)
{
    VALUE members, *ptr, *ptr_members;
    long i, len;

    members = rb_struct_members(s);
    len = RARRAY_LEN(members);
    rb_struct_modify(s);
    if (RSTRUCT_LEN(s) != len) {
	rb_raise(rb_eTypeError, "struct size differs (%ld required %ld given)",
		 len, RSTRUCT_LEN(s));
    }
    ptr = RSTRUCT_PTR(s);
    ptr_members = RARRAY_PTR(members);
    for (i=0; i<len; i++) {
	if (SYM2ID(ptr_members[i]) == id) {
	    ptr[i] = val;
	    return val;
	}
    }
    rb_name_error(id, "no member '%s' in struct", rb_id2name(id));

    UNREACHABLE;
}

/*
 *  call-seq:
 *     struct[symbol] = obj    -> obj
 *     struct[fixnum] = obj    -> obj
 *
 *  Attribute Assignment---Assigns to the instance variable named by
 *  <i>symbol</i> or <i>fixnum</i> the value <i>obj</i> and
 *  returns it. Will raise a <code>NameError</code> if the named
 *  variable does not exist, or an <code>IndexError</code> if the index
 *  is out of range.
 *
 *     Customer = Struct.new(:name, :address, :zip)
 *     joe = Customer.new("Joe Smith", "123 Maple, Anytown NC", 12345)
 *
 *     joe["name"] = "Luke"
 *     joe[:zip]   = "90210"
 *
 *     joe.name   #=> "Luke"
 *     joe.zip    #=> "90210"
 */

VALUE
rb_struct_aset(VALUE s, VALUE idx, VALUE val)
{
    long i;

    if (RB_TYPE_P(idx, T_SYMBOL)) {
	return rb_struct_aset_id(s, SYM2ID(idx), val);
    }
    if (RB_TYPE_P(idx, T_STRING)) {
	ID id = rb_check_id(&idx);
	if (!id) {
	    rb_name_error_str(idx, "no member '%"PRIsVALUE"' in struct",
			      QUOTE(idx));
	}
	return rb_struct_aset_id(s, id, val);
    }

    i = NUM2LONG(idx);
    if (i < 0) i = RSTRUCT_LEN(s) + i;
    if (i < 0) {
        rb_raise(rb_eIndexError, "offset %ld too small for struct(size:%ld)",
		 i, RSTRUCT_LEN(s));
    }
    if (RSTRUCT_LEN(s) <= i) {
        rb_raise(rb_eIndexError, "offset %ld too large for struct(size:%ld)",
		 i, RSTRUCT_LEN(s));
    }
    rb_struct_modify(s);
    return RSTRUCT_PTR(s)[i] = val;
}

static VALUE
struct_entry(VALUE s, long n)
{
    return rb_struct_aref(s, LONG2NUM(n));
}

/*
 * call-seq:
 *   struct.values_at(selector,... )  -> an_array
 *
 *   Returns an array containing the elements in
 *   +self+ corresponding to the given selector(s). The selectors
 *   may be either integer indices or ranges.
 *   See also </code>.select<code>.
 *
 *      a = %w{ a b c d e f }
 *      a.values_at(1, 3, 5)
 *      a.values_at(1, 3, 5, 7)
 *      a.values_at(-1, -3, -5, -7)
 *      a.values_at(1..3, 2...5)
 */

static VALUE
rb_struct_values_at(int argc, VALUE *argv, VALUE s)
{
    return rb_get_values_at(s, RSTRUCT_LEN(s), argc, argv, struct_entry);
}

/*
 *  call-seq:
 *     struct.select {|i| block }    -> array
 *     struct.select                 -> an_enumerator
 *
 *  Invokes the block passing in successive elements from
 *  <i>struct</i>, returning an array containing those elements
 *  for which the block returns a true value (equivalent to
 *  <code>Enumerable#select</code>).
 *
 *     Lots = Struct.new(:a, :b, :c, :d, :e, :f)
 *     l = Lots.new(11, 22, 33, 44, 55, 66)
 *     l.select {|v| (v % 2).zero? }   #=> [22, 44, 66]
 */

static VALUE
rb_struct_select(int argc, VALUE *argv, VALUE s)
{
    VALUE result;
    long i;

    rb_check_arity(argc, 0, 0);
    RETURN_SIZED_ENUMERATOR(s, 0, 0, rb_struct_size);
    result = rb_ary_new();
    for (i = 0; i < RSTRUCT_LEN(s); i++) {
	if (RTEST(rb_yield(RSTRUCT_PTR(s)[i]))) {
	    rb_ary_push(result, RSTRUCT_PTR(s)[i]);
	}
    }

    return result;
}

static VALUE
recursive_equal(VALUE s, VALUE s2, int recur)
{
    VALUE *ptr, *ptr2;
    long i, len;

    if (recur) return Qtrue; /* Subtle! */
    ptr = RSTRUCT_PTR(s);
    ptr2 = RSTRUCT_PTR(s2);
    len = RSTRUCT_LEN(s);
    for (i=0; i<len; i++) {
	if (!rb_equal(ptr[i], ptr2[i])) return Qfalse;
    }
    return Qtrue;
}

/*
 *  call-seq:
 *     struct == other_struct     -> true or false
 *
 *  Equality---Returns <code>true</code> if <i>other_struct</i> is
 *  equal to this one: they must be of the same class as generated by
 *  <code>Struct::new</code>, and the values of all instance variables
 *  must be equal (according to <code>Object#==</code>).
 *
 *     Customer = Struct.new(:name, :address, :zip)
 *     joe   = Customer.new("Joe Smith", "123 Maple, Anytown NC", 12345)
 *     joejr = Customer.new("Joe Smith", "123 Maple, Anytown NC", 12345)
 *     jane  = Customer.new("Jane Doe", "456 Elm, Anytown NC", 12345)
 *     joe == joejr   #=> true
 *     joe == jane    #=> false
 */

static VALUE
rb_struct_equal(VALUE s, VALUE s2)
{
    if (s == s2) return Qtrue;
    if (!RB_TYPE_P(s2, T_STRUCT)) return Qfalse;
    if (rb_obj_class(s) != rb_obj_class(s2)) return Qfalse;
    if (RSTRUCT_LEN(s) != RSTRUCT_LEN(s2)) {
	rb_bug("inconsistent struct"); /* should never happen */
    }

    return rb_exec_recursive_paired(recursive_equal, s, s2, s2);
}

static VALUE
recursive_hash(VALUE s, VALUE dummy, int recur)
{
    long i, len;
    st_index_t h;
    VALUE n, *ptr;

    h = rb_hash_start(rb_hash(rb_obj_class(s)));
    if (!recur) {
	ptr = RSTRUCT_PTR(s);
	len = RSTRUCT_LEN(s);
	for (i = 0; i < len; i++) {
	    n = rb_hash(ptr[i]);
	    h = rb_hash_uint(h, NUM2LONG(n));
	}
    }
    h = rb_hash_end(h);
    return INT2FIX(h);
}

/*
 * call-seq:
 *   struct.hash   -> fixnum
 *
 * Return a hash value based on this struct's contents.
 */

static VALUE
rb_struct_hash(VALUE s)
{
    return rb_exec_recursive_outer(recursive_hash, s, 0);
}

static VALUE
recursive_eql(VALUE s, VALUE s2, int recur)
{
    VALUE *ptr, *ptr2;
    long i, len;

    if (recur) return Qtrue; /* Subtle! */
    ptr = RSTRUCT_PTR(s);
    ptr2 = RSTRUCT_PTR(s2);
    len = RSTRUCT_LEN(s);
    for (i=0; i<len; i++) {
	if (!rb_eql(ptr[i], ptr2[i])) return Qfalse;
    }
    return Qtrue;
}

/*
 * call-seq:
 *   struct.eql?(other)   -> true or false
 *
 * Two structures are equal if they are the same object, or if all their
 * fields are equal (using <code>eql?</code>).
 */

static VALUE
rb_struct_eql(VALUE s, VALUE s2)
{
    if (s == s2) return Qtrue;
    if (!RB_TYPE_P(s2, T_STRUCT)) return Qfalse;
    if (rb_obj_class(s) != rb_obj_class(s2)) return Qfalse;
    if (RSTRUCT_LEN(s) != RSTRUCT_LEN(s2)) {
	rb_bug("inconsistent struct"); /* should never happen */
    }

    return rb_exec_recursive_paired(recursive_eql, s, s2, s2);
}

/*
 *  call-seq:
 *     struct.length    -> fixnum
 *     struct.size      -> fixnum
 *
 *  Returns the number of instance variables.
 *
 *     Customer = Struct.new(:name, :address, :zip)
 *     joe = Customer.new("Joe Smith", "123 Maple, Anytown NC", 12345)
 *     joe.length   #=> 3
 */

static VALUE
rb_struct_size(VALUE s)
{
    return LONG2FIX(RSTRUCT_LEN(s));
}

/*
 *  A <code>Struct</code> is a convenient way to bundle a number of
 *  attributes together, using accessor methods, without having to write
 *  an explicit class.
 *
 *  The <code>Struct</code> class is a generator of specific classes,
 *  each one of which is defined to hold a set of variables and their
 *  accessors. In these examples, we'll call the generated class
 *  ``<i>Customer</i>Class,'' and we'll show an example instance of that
 *  class as ``<i>Customer</i>Inst.''
 *
 *  In the descriptions that follow, the parameter <i>symbol</i> refers
 *  to a symbol, which is either a quoted string or a
 *  <code>Symbol</code> (such as <code>:name</code>).
 */
void
Init_Struct(void)
{
    rb_cStruct = rb_define_class("Struct", rb_cObject);
    rb_include_module(rb_cStruct, rb_mEnumerable);

    rb_undef_alloc_func(rb_cStruct);
    rb_define_singleton_method(rb_cStruct, "new", rb_struct_s_def, -1);

    rb_define_method(rb_cStruct, "initialize", rb_struct_initialize_m, -1);
    rb_define_method(rb_cStruct, "initialize_copy", rb_struct_init_copy, 1);

    rb_define_method(rb_cStruct, "==", rb_struct_equal, 1);
    rb_define_method(rb_cStruct, "eql?", rb_struct_eql, 1);
    rb_define_method(rb_cStruct, "hash", rb_struct_hash, 0);

    rb_define_method(rb_cStruct, "inspect", rb_struct_inspect, 0);
    rb_define_alias(rb_cStruct,  "to_s", "inspect");
    rb_define_method(rb_cStruct, "to_a", rb_struct_to_a, 0);
    rb_define_method(rb_cStruct, "to_h", rb_struct_to_h, 0);
    rb_define_method(rb_cStruct, "values", rb_struct_to_a, 0);
    rb_define_method(rb_cStruct, "size", rb_struct_size, 0);
    rb_define_method(rb_cStruct, "length", rb_struct_size, 0);

    rb_define_method(rb_cStruct, "each", rb_struct_each, 0);
    rb_define_method(rb_cStruct, "each_pair", rb_struct_each_pair, 0);
    rb_define_method(rb_cStruct, "[]", rb_struct_aref, 1);
    rb_define_method(rb_cStruct, "[]=", rb_struct_aset, 2);
    rb_define_method(rb_cStruct, "select", rb_struct_select, -1);
    rb_define_method(rb_cStruct, "values_at", rb_struct_values_at, -1);

    rb_define_method(rb_cStruct, "members", rb_struct_members_m, 0);
    id_members = rb_intern("__members__");
}
