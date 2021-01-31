/*
 *
 * MIT License
 *
 * Copyright (c) 2018 Richard Knight
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifndef FAST_OBJ_HDR
#define FAST_OBJ_HDR


typedef struct
{
    /* Texture name from .mtl file */
    char*                       name;

} fastObjTexture;


typedef struct
{
    /* Material name */
    char*                       name;

    /* Parameters */
    float                       Ka[3];  /* Ambient */
    float                       Kd[3];  /* Diffuse */
    float                       Ks[3];  /* Specular */
    float                       Ke[3];  /* Emission */
    float                       Kt[3];  /* Transmittance */
    float                       Ns;     /* Shininess */
    float                       Ni;     /* Index of refraction */
    float                       Tf[3];  /* Transmission filter */
    float                       d;      /* Disolve (alpha) */
    int                         illum;  /* Illumination model */

    /* Texture maps */
    fastObjTexture              map_Ka;
    fastObjTexture              map_Kd;
    fastObjTexture              map_Ks;
    fastObjTexture              map_Ke;
    fastObjTexture              map_Kt;
    fastObjTexture              map_Ns;
    fastObjTexture              map_Ni;
    fastObjTexture              map_d;
    fastObjTexture              map_bump;

} fastObjMaterial;

/* Allows user override to bigger indexable array */
#ifndef FAST_OBJ_UINT_TYPE
#define FAST_OBJ_UINT_TYPE unsigned int
#endif

typedef FAST_OBJ_UINT_TYPE fastObjUInt;

typedef struct
{
    fastObjUInt                 p;
    fastObjUInt                 t;
    fastObjUInt                 n;

} fastObjIndex;


typedef struct
{
    /* Group name */
    char*                       name;

    /* Number of faces */
    unsigned int                face_count;

    /* First face in fastObjMesh face_* arrays */
    unsigned int                face_offset;

    /* First index in fastObjMesh indices array */
    unsigned int                index_offset;

} fastObjGroup;


typedef struct
{
    /* Vertex data */
    unsigned int                position_count;
    float*                      positions;

    unsigned int                texcoord_count;
    float*                      texcoords;

    unsigned int                normal_count;
    float*                      normals;

    /* Face data: one element for each face */
    unsigned int                face_count;
    unsigned int*               face_vertices;
    unsigned int*               face_materials;

    /* Index data: one element for each face vertex */
    fastObjIndex*               indices;

    /* Materials */
    unsigned int                material_count;
    fastObjMaterial*            materials;

    /* Mesh groups */
    unsigned int                group_count;
    fastObjGroup*               groups;

    /* Mesh groups */
    unsigned int                mtllib_count;
    char**                      mtllibs;

} fastObjMesh;

#ifdef __cplusplus
extern "C" {
#endif

fastObjMesh*                    fast_obj_read(const char* buffer, unsigned int buffer_size);
int                             fast_obj_mtllib_read(fastObjMesh* mesh, const char* buffer, unsigned int buffer_size);
void                            fast_obj_destroy(fastObjMesh* mesh);

#ifdef __cplusplus
}
#endif

#endif


#ifdef FAST_OBJ_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef FAST_OBJ_REALLOC
#define FAST_OBJ_REALLOC        realloc
#endif

#ifndef FAST_OBJ_FREE
#define FAST_OBJ_FREE           free
#endif

#ifdef _WIN32
#define FAST_OBJ_SEPARATOR      '\\'
#define FAST_OBJ_OTHER_SEP      '/'
#else
#define FAST_OBJ_SEPARATOR      '/'
#define FAST_OBJ_OTHER_SEP      '\\'
#endif


/* Size of buffer to read into */
#define BUFFER_SIZE             65536

/* Max supported power when parsing float */
#define MAX_POWER               20

typedef struct
{
    /* Final mesh */
    fastObjMesh*                mesh;

    /* Current group */
    fastObjGroup                group;

    /* Current material index */
    unsigned int                material;

    /* Current line in file */
    unsigned int                line;

} fastObjData;


static const
double POWER_10_POS[MAX_POWER] =
{
    1.0e0,  1.0e1,  1.0e2,  1.0e3,  1.0e4,  1.0e5,  1.0e6,  1.0e7,  1.0e8,  1.0e9,
    1.0e10, 1.0e11, 1.0e12, 1.0e13, 1.0e14, 1.0e15, 1.0e16, 1.0e17, 1.0e18, 1.0e19,
};

static const
double POWER_10_NEG[MAX_POWER] =
{
    1.0e0,   1.0e-1,  1.0e-2,  1.0e-3,  1.0e-4,  1.0e-5,  1.0e-6,  1.0e-7,  1.0e-8,  1.0e-9,
    1.0e-10, 1.0e-11, 1.0e-12, 1.0e-13, 1.0e-14, 1.0e-15, 1.0e-16, 1.0e-17, 1.0e-18, 1.0e-19,
};


static void* memory_realloc(void* ptr, size_t bytes)
{
    return FAST_OBJ_REALLOC(ptr, bytes);
}


static
void memory_dealloc(void* ptr)
{
    FAST_OBJ_FREE(ptr);
}


#define array_clean(_arr)       ((_arr) ? memory_dealloc(_array_header(_arr)), 0 : 0)
#define array_push(_arr, _val)  (_array_mgrow(_arr, 1) ? ((_arr)[_array_size(_arr)++] = (_val), _array_size(_arr) - 1) : 0)
#define array_size(_arr)        ((_arr) ? _array_size(_arr) : 0)
#define array_capacity(_arr)    ((_arr) ? _array_capacity(_arr) : 0)
#define array_empty(_arr)       (array_size(_arr) == 0)

#define _array_header(_arr)     ((fastObjUInt*)(_arr)-2)
#define _array_size(_arr)       (_array_header(_arr)[0])
#define _array_capacity(_arr)   (_array_header(_arr)[1])
#define _array_ngrow(_arr, _n)  ((_arr) == 0 || (_array_size(_arr) + (_n) >= _array_capacity(_arr)))
#define _array_mgrow(_arr, _n)  (_array_ngrow(_arr, _n) ? (_array_grow(_arr, _n) != 0) : 1)
#define _array_grow(_arr, _n)   (*((void**)&(_arr)) = array_realloc(_arr, _n, sizeof(*(_arr))))


static void* array_realloc(void* ptr, fastObjUInt n, fastObjUInt b)
{
    fastObjUInt sz = array_size(ptr);
    fastObjUInt nsz = sz + n;
    fastObjUInt cap = array_capacity(ptr);
    fastObjUInt ncap = 3 * cap / 2;
    fastObjUInt* r;


    if (ncap < nsz)
        ncap = nsz;
    ncap = (ncap + 15) & ~15u;

    r = (fastObjUInt*)(memory_realloc(ptr ? _array_header(ptr) : 0, b * ncap + 2 * sizeof(fastObjUInt)));
    if (!r)
        return 0;

    r[0] = sz;
    r[1] = ncap;

    return (r + 2);
}


static
char* string_copy(const char* s, const char* e)
{
    size_t n;
    char*  p;
        
    n = (size_t)(e - s);
    p = (char*)(memory_realloc(0, n + 1));
    if (p)
    {
        memcpy(p, s, n);
        p[n] = '\0';
    }

    return p;
}


static
int string_equal(const char* a, const char* s, const char* e)
{
    size_t an = strlen(a);
    size_t sn = (size_t)(e - s);

    return an == sn && memcmp(a, s, an) == 0;
}


static
void string_fix_separators(char* s)
{
    while (*s)
    {
        if (*s == FAST_OBJ_OTHER_SEP)
            *s = FAST_OBJ_SEPARATOR;
        s++;
    }
}


static
int is_whitespace(char c)
{
    return (c == ' ' || c == '\t' || c == '\r');
}

static
int is_end_of_name(char c)
{
    return (c == '\t' || c == '\r' || c == '\n');
}

static
int is_newline(char c)
{
    return (c == '\n');
}


static
int is_digit(char c)
{
    return (c >= '0' && c <= '9');
}


static
int is_exponent(char c)
{
    return (c == 'e' || c == 'E');
}


static
const char* skip_whitespace(const char* ptr)
{
    while (is_whitespace(*ptr))
        ptr++;

    return ptr;
}


static
const char* skip_line(const char* ptr)
{
    while (!is_newline(*ptr++))
        ;

    return ptr;
}


static
fastObjGroup group_default(void)
{
    fastObjGroup group;

    group.name         = 0;
    group.face_count   = 0;
    group.face_offset  = 0;
    group.index_offset = 0;

    return group;
}


static
void group_clean(fastObjGroup* group)
{
    memory_dealloc(group->name);
}


static
void flush_output(fastObjData* data)
{
    /* Add group if not empty */
    if (data->group.face_count > 0)
        array_push(data->mesh->groups, data->group);
    else
        group_clean(&data->group);

    /* Reset for more data */
    data->group = group_default();
    data->group.face_offset = array_size(data->mesh->face_vertices);
    data->group.index_offset = array_size(data->mesh->indices);
}


static
const char* parse_int(const char* ptr, int* val)
{
    int sign;
    int num;


    if (*ptr == '-')
    {
        sign = -1;
        ptr++;
    }
    else
    {
        sign = +1;
    }

    num = 0;
    while (is_digit(*ptr))
        num = 10 * num + (*ptr++ - '0');

    *val = sign * num;

    return ptr;
}


static
const char* parse_float(const char* ptr, float* val)
{
    double        sign;
    double        num;
    double        fra;
    double        div;
    int           eval;
    const double* powers;


    ptr = skip_whitespace(ptr);

    switch (*ptr)
    {
    case '+':
        sign = 1.0;
        ptr++;
        break;

    case '-':
        sign = -1.0;
        ptr++;
        break;

    default:
        sign = 1.0;
        break;
    }


    num = 0.0;
    while (is_digit(*ptr))
        num = 10.0 * num + (double)(*ptr++ - '0');

    if (*ptr == '.')
        ptr++;

    fra = 0.0;
    div = 1.0;

    while (is_digit(*ptr))
    {
        fra  = 10.0 * fra + (double)(*ptr++ - '0');
        div *= 10.0;
    }

    num += fra / div;

    if (is_exponent(*ptr))
    {
        ptr++;

        switch (*ptr)
        {
        case '+':
            powers = POWER_10_POS;
            ptr++;
            break;

        case '-':
            powers = POWER_10_NEG;
            ptr++;
            break;

        default:
            powers = POWER_10_POS;
            break;
        }

        eval = 0;
        while (is_digit(*ptr))
            eval = 10 * eval + (*ptr++ - '0');

        num *= (eval >= MAX_POWER) ? 0.0 : powers[eval];
    }

    *val = (float)(sign * num);

    return ptr;
}


static
const char* parse_vertex(fastObjData* data, const char* ptr)
{
    unsigned int ii;
    float        v;


    for (ii = 0; ii < 3; ii++)
    {
        ptr = parse_float(ptr, &v);
        array_push(data->mesh->positions, v);
    }

    return ptr;
}


static
const char* parse_texcoord(fastObjData* data, const char* ptr)
{
    unsigned int ii;
    float        v;


    for (ii = 0; ii < 2; ii++)
    {
        ptr = parse_float(ptr, &v);
        array_push(data->mesh->texcoords, v);
    }

    return ptr;
}


static
const char* parse_normal(fastObjData* data, const char* ptr)
{
    unsigned int ii;
    float        v;


    for (ii = 0; ii < 3; ii++)
    {
        ptr = parse_float(ptr, &v);
        array_push(data->mesh->normals, v);
    }

    return ptr;
}


static
const char* parse_face(fastObjData* data, const char* ptr)
{
    unsigned int count;
    fastObjIndex vn;
    int          v;
    int          t;
    int          n;


    ptr = skip_whitespace(ptr);

    count = 0;
    while (!is_newline(*ptr))
    {
        v = 0;
        t = 0;
        n = 0;

        ptr = parse_int(ptr, &v);
        if (*ptr == '/')
        {
            ptr++;
            if (*ptr != '/')
                ptr = parse_int(ptr, &t);

            if (*ptr == '/')
            {
                ptr++;
                ptr = parse_int(ptr, &n);
            }
        }

        if (v < 0)
            vn.p = (array_size(data->mesh->positions) / 3) - (fastObjUInt)(-v);
        else
            vn.p = (fastObjUInt)(v);

        if (t < 0)
            vn.t = (array_size(data->mesh->texcoords) / 2) - (fastObjUInt)(-t);
        else if (t > 0)
            vn.t = (fastObjUInt)(t);
        else
            vn.t = 0;

        if (n < 0)
            vn.n = (array_size(data->mesh->normals) / 3) - (fastObjUInt)(-n);
        else if (n > 0)
            vn.n = (fastObjUInt)(n);
        else
            vn.n = 0;

        array_push(data->mesh->indices, vn);
        count++;

        ptr = skip_whitespace(ptr);
    }

    array_push(data->mesh->face_vertices, count);
    array_push(data->mesh->face_materials, data->material);

    data->group.face_count++;

    return ptr;
}


static
const char* parse_group(fastObjData* data, const char* ptr)
{
    const char* s;
    const char* e;


    ptr = skip_whitespace(ptr);

    s = ptr;
    while (!is_end_of_name(*ptr))
        ptr++;

    e = ptr;

    flush_output(data);
    data->group.name = string_copy(s, e);

    return ptr;
}


static
fastObjTexture map_default(void)
{
    fastObjTexture map;

    map.name = 0;

    return map;
}


static
fastObjMaterial mtl_default(void)
{
    fastObjMaterial mtl;

    mtl.name = 0;

    mtl.Ka[0] = 0.0;
    mtl.Ka[1] = 0.0;
    mtl.Ka[2] = 0.0;
    mtl.Kd[0] = 1.0;
    mtl.Kd[1] = 1.0;
    mtl.Kd[2] = 1.0;
    mtl.Ks[0] = 0.0;
    mtl.Ks[1] = 0.0;
    mtl.Ks[2] = 0.0;
    mtl.Ke[0] = 0.0;
    mtl.Ke[1] = 0.0;
    mtl.Ke[2] = 0.0;
    mtl.Kt[0] = 0.0;
    mtl.Kt[1] = 0.0;
    mtl.Kt[2] = 0.0;
    mtl.Ns    = 1.0;
    mtl.Ni    = 1.0;
    mtl.Tf[0] = 1.0;
    mtl.Tf[1] = 1.0;
    mtl.Tf[2] = 1.0;
    mtl.d     = 1.0;
    mtl.illum = 1;

    mtl.map_Ka   = map_default();
    mtl.map_Kd   = map_default();
    mtl.map_Ks   = map_default();
    mtl.map_Ke   = map_default();
    mtl.map_Kt   = map_default();
    mtl.map_Ns   = map_default();
    mtl.map_Ni   = map_default();
    mtl.map_d    = map_default();
    mtl.map_bump = map_default();

    return mtl;
}


static
unsigned int find_or_add_mtl(fastObjMesh* mesh, const char* s, const char* e)
{
    unsigned int        idx;
    fastObjMaterial*    mtl;


    idx = 0;

    /* Find an existing material with the same name */
    while (idx < array_size(mesh->materials))
    {
        mtl = &mesh->materials[idx];
        if (mtl->name && string_equal(mtl->name, s, e))
            break;

        idx++;
    }

    /* If no material found, create a new one. */
    if (idx == array_size(mesh->materials)) 
    {
        array_push(mesh->materials, mtl_default());
        mesh->materials[idx].name = string_copy(s, e);
    }

    return idx;
}


static
const char* parse_usemtl(fastObjData* data, const char* ptr)
{
    const char*      s;
    const char*      e;


    ptr = skip_whitespace(ptr);

    /* Parse the material name */
    s = ptr;
    while (!is_end_of_name(*ptr))
        ptr++;

    e = ptr;

    data->material = find_or_add_mtl(data->mesh, s, e);

    return ptr;
}


static
void map_clean(fastObjTexture* map)
{
    memory_dealloc(map->name);
}


static
void mtl_clean(fastObjMaterial* mtl)
{
    map_clean(&mtl->map_Ka);
    map_clean(&mtl->map_Kd);
    map_clean(&mtl->map_Ks);
    map_clean(&mtl->map_Ke);
    map_clean(&mtl->map_Kt);
    map_clean(&mtl->map_Ns);
    map_clean(&mtl->map_Ni);
    map_clean(&mtl->map_d);
    map_clean(&mtl->map_bump);

    memory_dealloc(mtl->name);
}


static
const char* read_mtl_int(const char* p, int* v)
{
    return parse_int(p, v);
}


static
const char* read_mtl_single(const char* p, float* v)
{
    return parse_float(p, v);
}


static
const char* read_mtl_triple(const char* p, float v[3])
{
    p = read_mtl_single(p, &v[0]);
    p = read_mtl_single(p, &v[1]);
    p = read_mtl_single(p, &v[2]);

    return p;
}


static
const char* read_map(const char* ptr, fastObjTexture* map)
{
    const char* s;
    const char* e;
    char*       name;

    ptr = skip_whitespace(ptr);

    /* Don't support options at present */
    if (*ptr == '-')
        return ptr;


    /* Read name */
    s = ptr;
    while (!is_end_of_name(*ptr))
        ptr++;

    e = ptr;

    name = string_copy(s, e);
    map->name = name;

    return e;
}


static
const char* parse_mtllib(fastObjData* data, const char* ptr)
{
    const char* s;
    const char* e;
    char*       path;


    ptr = skip_whitespace(ptr);

    s = ptr;
    while (!is_end_of_name(*ptr))
        ptr++;

    e = ptr;

    path = string_copy(s, e);
    if (path)
    {
        string_fix_separators(path);
        array_push(data->mesh->mtllibs, path);
    }

    return ptr;
}


static
void parse_buffer(fastObjData* data, const char* ptr, const char* end)
{
    const char* p;
    
    
    p = ptr;
    while (p != end)
    {
        p = skip_whitespace(p);

        switch (*p)
        {
        case 'v':
            p++;

            switch (*p++)
            {
            case ' ':
            case '\t':
                p = parse_vertex(data, p);
                break;

            case 't':
                p = parse_texcoord(data, p);
                break;

            case 'n':
                p = parse_normal(data, p);
                break;

            default:
                p--; /* roll p++ back in case *p was a newline */
            }
            break;

        case 'f':
            p++;

            switch (*p++)
            {
            case ' ':
            case '\t':
                p = parse_face(data, p);
                break;

            default:
                p--; /* roll p++ back in case *p was a newline */
            }
            break;

        case 'g':
            p++;

            switch (*p++)
            {
            case ' ':
            case '\t':
                p = parse_group(data, p);
                break;

            default:
                p--; /* roll p++ back in case *p was a newline */
            }
            break;

        case 'm':
            p++;
            if (p[0] == 't' &&
                p[1] == 'l' &&
                p[2] == 'l' &&
                p[3] == 'i' &&
                p[4] == 'b' &&
                is_whitespace(p[5]))
                p = parse_mtllib(data, p + 5);
            break;

        case 'u':
            p++;
            if (p[0] == 's' &&
                p[1] == 'e' &&
                p[2] == 'm' &&
                p[3] == 't' &&
                p[4] == 'l' &&
                is_whitespace(p[5]))
                p = parse_usemtl(data, p + 5);
            break;

        case '#':
            break;
        }

        p = skip_line(p);

        data->line++;
    }
}


void fast_obj_destroy(fastObjMesh* m)
{
    unsigned int ii;


    for (ii = 0; ii < array_size(m->groups); ii++)
        group_clean(&m->groups[ii]);

    for (ii = 0; ii < array_size(m->materials); ii++)
        mtl_clean(&m->materials[ii]);

    for (ii = 0; ii < array_size(m->mtllibs); ii++)
        memory_dealloc(m->mtllibs[ii]);

    array_clean(m->positions);
    array_clean(m->texcoords);
    array_clean(m->normals);
    array_clean(m->face_vertices);
    array_clean(m->face_materials);
    array_clean(m->indices);
    array_clean(m->groups);
    array_clean(m->materials);
    array_clean(m->mtllibs);

    memory_dealloc(m);
}


int fast_obj_mtllib_read(fastObjMesh* mesh, const char* buffer, unsigned int buffer_size)
{
    const char*         s;
    const char*         p;
    const char*         e;
    int                 found_d;
    fastObjMaterial*    mtl;

    /* Ensure buffer ends in a newline */
    if (buffer[buffer_size - 1] != '\n')
        return 0;

    found_d = 0;

    p = buffer;
    e = buffer + buffer_size;

  

    while (p < e)
    {
        p = skip_whitespace(p);

        switch (*p)
        {
        case 'n':
            p++;
            if (p[0] == 'e' &&
                p[1] == 'w' &&
                p[2] == 'm' &&
                p[3] == 't' &&
                p[4] == 'l' &&
                is_whitespace(p[5]))
            {
                /* Read name */
                p += 5;

                while (is_whitespace(*p))
                    p++;

                s = p;
                while (!is_end_of_name(*p))
                    p++;

                mtl = &mesh->materials[find_or_add_mtl(mesh, s, p)];
            }
            break;

        case 'K':
            if (p[1] == 'a')
                p = read_mtl_triple(p + 2, mtl->Ka);
            else if (p[1] == 'd')
                p = read_mtl_triple(p + 2, mtl->Kd);
            else if (p[1] == 's')
                p = read_mtl_triple(p + 2, mtl->Ks);
            else if (p[1] == 'e')
                p = read_mtl_triple(p + 2, mtl->Ke);
            else if (p[1] == 't')
                p = read_mtl_triple(p + 2, mtl->Kt);
            break;

        case 'N':
            if (p[1] == 's')
                p = read_mtl_single(p + 2, &mtl->Ns);
            else if (p[1] == 'i')
                p = read_mtl_single(p + 2, &mtl->Ni);
            break;

        case 'T':
            if (p[1] == 'r')
            {
                float Tr;
                p = read_mtl_single(p + 2, &Tr);
                if (!found_d)
                {
                    /* Ignore Tr if we've already read d */
                    mtl->d = 1.0f - Tr;
                }
            }
            else if (p[1] == 'f')
                p = read_mtl_triple(p + 2, mtl->Tf);
            break;

        case 'd':
            if (is_whitespace(p[1]))
            {
                p = read_mtl_single(p + 1, &mtl->d);
                found_d = 1;
            }
            break;

        case 'i':
            p++;
            if (p[0] == 'l' &&
                p[1] == 'l' &&
                p[2] == 'u' &&
                p[3] == 'm' &&
                is_whitespace(p[4]))
            {
                p = read_mtl_int(p + 4, &mtl->illum);
            }
            break;

        case 'm':
            p++;
            if (p[0] == 'a' &&
                p[1] == 'p' &&
                p[2] == '_')
            {
                p += 3;
                if (*p == 'K')
                {
                    p++;
                    if (is_whitespace(p[1]))
                    {
                        if (*p == 'a')
                            p = read_map(p + 1, &mtl->map_Ka);
                        else if (*p == 'd')
                            p = read_map(p + 1, &mtl->map_Kd);
                        else if (*p == 's')
                            p = read_map(p + 1, &mtl->map_Ks);
                        else if (*p == 'e')
                            p = read_map(p + 1, &mtl->map_Ke);
                        else if (*p == 't')
                            p = read_map(p + 1, &mtl->map_Kt);
                    }
                }
                else if (*p == 'N')
                {
                    p++;
                    if (is_whitespace(p[1]))
                    {
                        if (*p == 's')
                            p = read_map(p + 1, &mtl->map_Ns);
                        else if (*p == 'i')
                            p = read_map(p + 1, &mtl->map_Ni);
                    }
                }
                else if (*p == 'd')
                {
                    p++;
                    if (is_whitespace(*p))
                        p = read_map(p, &mtl->map_d);
                }
                else if (p[0] == 'B' &&
                         p[1] == 'u' &&
                         p[2] == 'm' &&
                         p[3] == 'p' &&
                         is_whitespace(p[4]))
                {
                    p = read_map(p + 4, &mtl->map_bump);
                }
            }
            break;

        case '#':
            break;
        }

        p = skip_line(p);
    }

    return 1;
}


fastObjMesh* fast_obj_read(const char* buffer, unsigned int buffer_size)
{
    fastObjData  data;
    fastObjMesh* m;
    const char*  start;
    const char*  end;

    /* Ensure buffer ends in a newline */
    if (buffer[buffer_size - 1] != '\n')
        return 0; 

    /* Empty mesh */
    m = (fastObjMesh*)(memory_realloc(0, sizeof(fastObjMesh)));
    if (!m)
        return 0;

    m->positions      = 0;
    m->texcoords      = 0;
    m->normals        = 0;
    m->face_vertices  = 0;
    m->face_materials = 0;
    m->indices        = 0;
    m->materials      = 0;
    m->groups         = 0;
    m->mtllibs        = 0;


    /* Add dummy position/texcoord/normal */
    array_push(m->positions, 0.0f);
    array_push(m->positions, 0.0f);
    array_push(m->positions, 0.0f);

    array_push(m->texcoords, 0.0f);
    array_push(m->texcoords, 0.0f);

    array_push(m->normals, 0.0f);
    array_push(m->normals, 0.0f);
    array_push(m->normals, 1.0f);


    /* Data needed during parsing */
    data.mesh           = m;
    data.group          = group_default();
    data.material       = 0;
    data.line           = 1;


    start = buffer;
    end = start + buffer_size;


    /* Process buffer */
    parse_buffer(&data, buffer, end);


    /* Flush final group */
    flush_output(&data);
    group_clean(&data.group);


    m->position_count = array_size(m->positions) / 3;
    m->texcoord_count = array_size(m->texcoords) / 2;
    m->normal_count   = array_size(m->normals) / 3;
    m->face_count     = array_size(m->face_vertices);
    m->material_count = array_size(m->materials);
    m->group_count    = array_size(m->groups);
    m->mtllib_count   = array_size(m->mtllibs);


    return m;
}

#endif
