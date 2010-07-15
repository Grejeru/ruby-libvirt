/*
 * secret.c: virSecret methods
 *
 * Copyright (C) 2010 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */

#include <ruby.h>
#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include "common.h"
#include "connect.h"
#include "extconf.h"

#if HAVE_TYPE_VIRSECRETPTR
static VALUE c_secret;

static void secret_free(void *s) {
    generic_free(Secret, s);
}

static virSecretPtr secret_get(VALUE s) {
    generic_get(Secret, s);
}

static VALUE secret_new(virSecretPtr s, VALUE conn) {
    return generic_new(c_secret, s, conn, secret_free);
}

/*
 * call-seq:
 *   conn.num_of_secrets -> fixnum
 *
 * Call +virConnectNumOfSecrets+[http://www.libvirt.org/html/libvirt-libvirt.html#virConnectNumOfSecrets]
 */
static VALUE libvirt_conn_num_of_secrets(VALUE s) {
    gen_conn_num_of(s, Secrets);
}

/*
 * call-seq:
 *   conn.list_secrets -> list
 *
 * Call +virConnectListSecrets+[http://www.libvirt.org/html/libvirt-libvirt.html#virConnectListSecrets]
 */
static VALUE libvirt_conn_list_secrets(VALUE s) {
    gen_conn_list_names(s, Secrets);
}

/*
 * call-seq:
 *   conn.lookup_secret_by_uuid -> Libvirt::Secret
 *
 * Call +virSecretLookupByUUID+[http://www.libvirt.org/html/libvirt-libvirt.html#virSecretLookupByUUID]
 */
static VALUE libvirt_conn_lookup_secret_by_uuid(VALUE c, VALUE uuid) {
    virSecretPtr secret;
    virConnectPtr conn = connect_get(c);

    secret = virSecretLookupByUUIDString(conn, StringValueCStr(uuid));
    _E(secret == NULL, create_error(e_RetrieveError, "virSecretLookupByUUID", "", conn));

    return secret_new(secret, c);
}

/*
 * call-seq:
 *   conn.lookup_secret_by_usage -> Libvirt::Secret
 *
 * Call +virSecretLookupByUsage+[http://www.libvirt.org/html/libvirt-libvirt.html#virSecretLookupByUsage]
 */
static VALUE libvirt_conn_lookup_secret_by_usage(int argc, VALUE *argv, VALUE c) {
    virSecretPtr secret;
    virConnectPtr conn = connect_get(c);
    VALUE usageType, usageID;

    rb_scan_args(argc, argv, "11", &usageType, &usageID);

    secret = virSecretLookupByUsage(conn, usageType, StringValueCStr(usageID));
    _E(secret == NULL, create_error(e_RetrieveError, "virSecretLookupByUsage", "", conn));

    return secret_new(secret, c);
}

/*
 * call-seq:
 *   conn.define_secret_xml -> Libvirt::Secret
 *
 * Call +virSecretDefineXML+[http://www.libvirt.org/html/libvirt-libvirt.html#virSecretDefineXML]
 */
static VALUE libvirt_conn_define_secret_xml(int argc, VALUE *argv, VALUE c) {
    virSecretPtr secret;
    virConnectPtr conn = connect_get(c);
    VALUE xml, flags;

    rb_scan_args(argc, argv, "11", &xml, &flags);

    if (NIL_P(flags))
        flags = INT2FIX(0);

    secret = virSecretDefineXML(conn, StringValueCStr(xml), NUM2UINT(flags));
    _E(secret == NULL, create_error(e_DefinitionError, "virSecretDefineXML", "", conn));

    return secret_new(secret, c);
}

/*
 * call-seq:
 *   secret.uuid -> string
 *
 * Call +virSecretGetUUIDString+[http://www.libvirt.org/html/libvirt-libvirt.html#virSecretGetUUIDString]
 */
static VALUE libvirt_secret_uuid(VALUE s) {
    virSecretPtr secret = secret_get(s);
    int r;
    char uuid[VIR_UUID_STRING_BUFLEN];

    r = virSecretGetUUIDString(secret, uuid);
    _E(r < 0, create_error(e_RetrieveError, "virSecretGetUUIDString", "", conn(s)));

    return rb_str_new2((char *)uuid);
}

/*
 * call-seq:
 *   secret.usagetype -> fixnum
 *
 * Call +virSecretGetUsageType+[http://www.libvirt.org/html/libvirt-libvirt.html#virSecretGetUsageType]
 */
static VALUE libvirt_secret_usagetype(VALUE s) {
    virSecretPtr secret = secret_get(s);
    int ret;

    ret = virSecretGetUsageType(secret);
    _E(ret < 0, create_error(e_RetrieveError, "virSecretGetUsageType", "", conn(s)));

    return INT2NUM(ret);
}

/*
 * call-seq:
 *   secret.usageid -> string
 *
 * Call +virSecretGetUsageID+[http://www.libvirt.org/html/libvirt-libvirt.html#virSecretGetUsageID]
 */
static VALUE libvirt_secret_usageid(VALUE s) {
    gen_call_string(virSecretGetUsageID, conn(s), 0, secret_get(s));
}

/*
 * call-seq:
 *   secret.xml_desc -> string
 *
 * Call +virSecretGetXMLDesc+[http://www.libvirt.org/html/libvirt-libvirt.html#virSecretGetXMLDesc]
 */
static VALUE libvirt_secret_xml_desc(int argc, VALUE *argv, VALUE s) {
    VALUE flags;

    rb_scan_args(argc, argv, "01", &flags);

    if (NIL_P(flags))
        flags = INT2FIX(0);

    gen_call_string(virSecretGetXMLDesc, conn(s), 1, secret_get(s),
                    NUM2UINT(flags));
}

/*
 * call-seq:
 *   secret.set_value -> nil
 *
 * Call +virSecretSetValue+[http://www.libvirt.org/html/libvirt-libvirt.html#virSecretSetValue]
 */
static VALUE libvirt_secret_set_value(int argc, VALUE *argv, VALUE s) {
    virSecretPtr secret = secret_get(s);
    VALUE flags;
    VALUE value;
    int r;

    rb_scan_args(argc, argv, "11", &value, &flags);

    if (NIL_P(flags))
        flags = INT2FIX(0);

    StringValue(value);

    r = virSecretSetValue(secret, (unsigned char *)RSTRING(value)->ptr,
                          RSTRING(value)->len, NUM2UINT(flags));

    _E(r < 0, create_error(e_RetrieveError, "virSecretSetValue", "", conn(s)));

    return Qnil;
}

/*
 * call-seq:
 *   secret.get_value -> string
 *
 * Call +virSecretGetValue+[http://www.libvirt.org/html/libvirt-libvirt.html#virSecretGetValue]
 */
static VALUE libvirt_secret_get_value(int argc, VALUE *argv, VALUE s) {
    virSecretPtr secret = secret_get(s);
    VALUE flags;
    unsigned char *ret;
    size_t value_size;

    rb_scan_args(argc, argv, "01", &flags);

    if (NIL_P(flags))
        flags = INT2FIX(0);

    ret = virSecretGetValue(secret, &value_size, NUM2UINT(flags));

    _E(ret == NULL, create_error(e_RetrieveError, "virSecretGetValue", "", conn(s)));

    return rb_str_new((char *)ret, value_size);
}

/*
 * call-seq:
 *   secret.undefine -> nil
 *
 * Call +virSecretUndefine+[http://www.libvirt.org/html/libvirt-libvirt.html#virSecretUndefine]
 */
static VALUE libvirt_secret_undefine(VALUE s) {
    gen_call_void(virSecretUndefine, conn(s), secret_get(s));
}

/*
 * call-seq:
 *   secret.free -> nil
 *
 * Call +virSecretFree+[http://www.libvirt.org/html/libvirt-libvirt.html#virSecretFree]
 */
static VALUE libvirt_secret_free(VALUE s) {
    gen_call_free(Secret, s);
}

#endif

/*
 * Class Libvirt::Secret
 */
void init_secret()
{
#if HAVE_TYPE_VIRSECRETPTR
    c_secret = rb_define_class_under(m_libvirt, "Secret", rb_cObject);

    rb_define_const(c_secret, "USAGE_TYPE_VOLUME",
                    INT2NUM(VIR_SECRET_USAGE_TYPE_VOLUME));
    rb_define_attr(c_secret, "connection", 1, 0);

    /* Secret lookup/creation methods */
    rb_define_method(c_connect, "num_of_secrets",
                     libvirt_conn_num_of_secrets, 0);
    rb_define_method(c_connect, "list_secrets",
                     libvirt_conn_list_secrets, 0);
    rb_define_method(c_connect, "lookup_secret_by_uuid",
                     libvirt_conn_lookup_secret_by_uuid, 1);
    rb_define_method(c_connect, "lookup_secret_by_usage",
                     libvirt_conn_lookup_secret_by_usage, -1);
    rb_define_method(c_connect, "define_secret_xml",
                     libvirt_conn_define_secret_xml, -1);

    /* Secret object methods */
    rb_define_method(c_secret, "uuid", libvirt_secret_uuid, 0);
    rb_define_method(c_secret, "usagetype", libvirt_secret_usagetype, 0);
    rb_define_method(c_secret, "usageid", libvirt_secret_usageid, 0);
    rb_define_method(c_secret, "xml_desc", libvirt_secret_xml_desc, -1);
    rb_define_method(c_secret, "set_value", libvirt_secret_set_value, -1);
    rb_define_method(c_secret, "get_value", libvirt_secret_get_value, -1);
    rb_define_method(c_secret, "undefine", libvirt_secret_undefine, 0);
    rb_define_method(c_secret, "free", libvirt_secret_free, 0);
#endif
}