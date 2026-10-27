#include <stdalign.h>
#include "internal.h"
#include "void.h"

#ifndef M_PIf
    #define M_PIf 3.14159265358979323846f
#endif

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

// ============================================================================
// TRIANGLE & QUAD
// ============================================================================

bool void_gfx_draw_triangle(const VoidWindow *window, const float x1, const float y1, const float x2, const float y2, const float x3, const float y3, const uint8 r, const uint8 g, const uint8 b, const uint8 a, const bool fill) {
    if (!window || !window->render) return false;
    if (!fill) {
        const VoidPoint pts[4] = {{.x = x1, .y = y1}, {.x = x2, .y = y2}, {.x = x3, .y = y3}, {.x = x1, .y = y1}};
        return void_render_lines(window, pts, 4, r, g, b, a);
    }
    const SDL_Color col = {.r = r, .g = g, .b = b, .a = a};
    const SDL_Vertex vertices[3] = {
        {.position = {.x = x1, .y = y1}, .color = col, .tex_coord = {.x = 0, .y = 0}},
        {.position = {.x = x2, .y = y2}, .color = col, .tex_coord = {.x = 0, .y = 0}},
        {.position = {.x = x3, .y = y3}, .color = col, .tex_coord = {.x = 0, .y = 0}}
    };
    return SDL_RenderGeometry(window->render, NULL, vertices, 3, NULL, 0) == 0;
}

bool void_gfx_draw_quad(const VoidWindow *window, const float x1, const float y1, const float x2, const float y2, const float x3, const float y3, const float x4, const float y4, const uint8 r, const uint8 g, const uint8 b, const uint8 a, const bool fill) {
    if (!window || !window->render) return false;
    if (!fill) {
        const VoidPoint pts[5] = {{.x = x1, .y = y1}, {.x = x2, .y = y2}, {.x = x3, .y = y3}, {.x = x4, .y = y4}, {.x = x1, .y = y1}};
        return void_render_lines(window, pts, 5, r, g, b, a);
    }
    const SDL_Color col = {.r = r, .g = g, .b = b, .a = a};
    const SDL_Vertex vertices[4] = {
        {.position = {.x = x1, .y = y1}, .color = col, .tex_coord = {.x = 0, .y = 0}},
        {.position = {.x = x2, .y = y2}, .color = col, .tex_coord = {.x = 0, .y = 0}},
        {.position = {.x = x3, .y = y3}, .color = col, .tex_coord = {.x = 0, .y = 0}},
        {.position = {.x = x4, .y = y4}, .color = col, .tex_coord = {.x = 0, .y = 0}}
    };
    const int indices[6] = {0, 1, 2, 0, 2, 3};
    return SDL_RenderGeometry(window->render, NULL, vertices, 4, indices, 6) == 0;
}

// ============================================================================
// CIRCLE (Adaptive & Scoped Zero-Allocation)
// ============================================================================

bool void_gfx_draw_circle(const VoidWindow *window, const float x, const float y, const float radius, const uint8 r, const uint8 g, const uint8 b, const uint8 a, const bool fill) {
    if (!window || !window->render || radius <= 0.0f) return false;
    int segments = (int)(radius * 1.5f);
    if (segments < 16) segments = 16;
    if (segments > 256) segments = 256;
    const float step = 2.0f * M_PIf / (float)segments;
    const VoidScope scope = void_arena_push(NULL);
    if (!fill) {
        VoidPoint *pts = void_arena_alloc(NULL, (segments + 1) * sizeof(VoidPoint), alignof(VoidPoint));
        if (!pts) {
            void_arena_pop(NULL, scope);
            return false;
        }
        for (int i = 0; i <= segments; ++i) {
            const float angle = (float)i * step;
            pts[i].x = x + cosf(angle) * radius;
            pts[i].y = y + sinf(angle) * radius;
        }
        const bool ok = void_render_lines(window, pts, segments + 1, r, g, b, a);
        void_arena_pop(NULL, scope);
        return ok;
    }
    const int num_vertices = segments + 1;
    const int num_indices = segments * 3;
    SDL_Vertex *vertices = void_arena_alloc(NULL, num_vertices * sizeof(SDL_Vertex), alignof(SDL_Vertex));
    int *indices = void_arena_alloc(NULL, num_indices * sizeof(int), alignof(int));
    if (!vertices || !indices) {
        void_arena_pop(NULL, scope);
        return false;
    }
    const SDL_Color col = {.r = r, .g = g, .b = b, .a = a};
    vertices[0].position.x = x;
    vertices[0].position.y = y;
    vertices[0].color = col;
    vertices[0].tex_coord.x = 0;
    vertices[0].tex_coord.y = 0;
    for (int i = 0; i < segments; ++i) {
        const float angle = (float)i * step;
        vertices[i + 1].position.x = x + cosf(angle) * radius;
        vertices[i + 1].position.y = y + sinf(angle) * radius;
        vertices[i + 1].color = col;
        vertices[i + 1].tex_coord.x = 0;
        vertices[i + 1].tex_coord.y = 0;
    }
    for (int i = 0; i < segments; ++i) {
        const int idx = i * 3;
        indices[idx] = 0;
        indices[idx + 1] = i + 1;
        indices[idx + 2] = (i + 1) % segments + 1;
    }
    const bool ok = SDL_RenderGeometry(window->render, NULL, vertices, num_vertices, indices, num_indices) == 0;
    void_arena_pop(NULL, scope);
    return ok;
}

// ============================================================================
// REGULAR POLYGONS
// ============================================================================

bool void_gfx_draw_regular_polygon(const VoidWindow *window, const float x, const float y, const float radius, const int sides, const float rotation, const uint8 r, const uint8 g, const uint8 b, const uint8 a, const bool fill) {
    if (!window || !window->render || radius <= 0.0f || sides < 3) return false;
    const float step = 2.0f * M_PIf / (float)sides;
    const VoidScope scope = void_arena_push(NULL);
    if (!fill) {
        VoidPoint *pts = void_arena_alloc(NULL, (sides + 1) * sizeof(VoidPoint), alignof(VoidPoint));
        if (!pts) {
            void_arena_pop(NULL, scope);
            return false;
        }
        for (int i = 0; i <= sides; ++i) {
            const float angle = rotation + (float)i * step;
            pts[i].x = x + cosf(angle) * radius;
            pts[i].y = y + sinf(angle) * radius;
        }
        const bool ok = void_render_lines(window, pts, sides + 1, r, g, b, a);
        void_arena_pop(NULL, scope);
        return ok;
    }
    const int num_vertices = sides + 1;
    const int num_indices = sides * 3;
    SDL_Vertex *vertices = void_arena_alloc(NULL, num_vertices * sizeof(SDL_Vertex), alignof(SDL_Vertex));
    int *indices = void_arena_alloc(NULL, num_indices * sizeof(int), alignof(int));
    if (!vertices || !indices) {
        void_arena_pop(NULL, scope);
        return false;
    }
    const SDL_Color col = {.r = r, .g = g, .b = b, .a = a};
    vertices[0].position.x = x;
    vertices[0].position.y = y;
    vertices[0].color = col;
    vertices[0].tex_coord.x = 0;
    vertices[0].tex_coord.y = 0;
    for (int i = 0; i < sides; ++i) {
        const float angle = rotation + (float)i * step;
        vertices[i + 1].position.x = x + cosf(angle) * radius;
        vertices[i + 1].position.y = y + sinf(angle) * radius;
        vertices[i + 1].color = col;
        vertices[i + 1].tex_coord.x = 0;
        vertices[i + 1].tex_coord.y = 0;
    }
    for (int i = 0; i < sides; ++i) {
        const int idx = i * 3;
        indices[idx] = 0;
        indices[idx + 1] = i + 1;
        indices[idx + 2] = (i + 1) % sides + 1;
    }
    const bool ok = SDL_RenderGeometry(window->render, NULL, vertices, num_vertices, indices, num_indices) == 0;
    void_arena_pop(NULL, scope);
    return ok;
}

// ============================================================================
// CONCAVE POLYGON (Ear Clipping Triangulation)
// ============================================================================

static float cross_product(const VoidPoint a, const VoidPoint b, const VoidPoint c) {
    return (b.x - a.x) * (c.y - b.y) - (b.y - a.y) * (c.x - b.x);
}

static bool point_in_triangle(const VoidPoint p, const VoidPoint a, const VoidPoint b, const VoidPoint c) {
    const float cp1 = cross_product(a, b, p);
    const float cp2 = cross_product(b, c, p);
    const float cp3 = cross_product(c, a, p);
    const bool has_neg = (cp1 < 0.0f) || (cp2 < 0.0f) || (cp3 < 0.0f);
    const bool has_pos = (cp1 > 0.0f) || (cp2 > 0.0f) || (cp3 > 0.0f);
    return !(has_neg && has_pos);
}

static bool is_polygon_convex(const VoidPoint *points, const int count) {
    if (count < 4) return true;
    bool sign = false;
    bool initialized = false;
    for (int i = 0; i < count; i++) {
        const int i1 = i;
        const int i2 = (i + 1) % count;
        const int i3 = (i + 2) % count;
        const float cp = cross_product(points[i1], points[i2], points[i3]);
        if (fabsf(cp) < 0.0001f) continue;
        const bool current_sign = cp > 0.0f;
        if (!initialized) {
            sign = current_sign;
            initialized = true;
        } else if (sign != current_sign) {
            return false;
        }
    }
    return true;
}

bool void_gfx_draw_polygon(const VoidWindow *window, const VoidPoint *points, const int count, const uint8 r, const uint8 g, const uint8 b, const uint8 a, const bool fill) {
    if (!window || !window->render || !points || count < 3) return false;
    const VoidScope scope = void_arena_push(NULL);
    if (!fill) {
        VoidPoint *pts = void_arena_alloc(NULL, (count + 1) * sizeof(VoidPoint), alignof(VoidPoint));
        if (!pts) {
            void_arena_pop(NULL, scope);
            return false;
        }
        for (int i = 0; i < count; ++i) {
            pts[i] = points[i];
        }
        pts[count] = points[0];
        const bool ok = void_render_lines(window, pts, count + 1, r, g, b, a);
        void_arena_pop(NULL, scope);
        return ok;
    }
    const int num_triangles = count - 2;
    const int num_indices = num_triangles * 3;
    SDL_Vertex *vertices = void_arena_alloc(NULL, count * sizeof(SDL_Vertex), alignof(SDL_Vertex));
    int *indices = void_arena_alloc(NULL, num_indices * sizeof(int), alignof(int));
    int *prev = void_arena_alloc(NULL, count * sizeof(int), alignof(int));
    int *next = void_arena_alloc(NULL, count * sizeof(int), alignof(int));
    if (!vertices || !indices || !prev || !next) {
        void_arena_pop(NULL, scope);
        return false;
    }
    const SDL_Color col = {.r = r, .g = g, .b = b, .a = a};
    float area = 0.0f;
    for (int i = 0; i < count; ++i) {
        vertices[i].position.x = points[i].x;
        vertices[i].position.y = points[i].y;
        vertices[i].color = col;
        vertices[i].tex_coord.x = 0;
        vertices[i].tex_coord.y = 0;
        prev[i] = (i - 1 + count) % count;
        next[i] = (i + 1) % count;
        area += points[i].x * points[next[i]].y - points[next[i]].x * points[i].y;
    }
    if (is_polygon_convex(points, count)) {
        int idx = 0;
        for (int i = 1; i < count - 1; ++i) {
            indices[idx++] = 0;
            indices[idx++] = i;
            indices[idx++] = i + 1;
        }
        const bool ok = SDL_RenderGeometry(window->render, NULL, vertices, count, indices, idx) == 0;
        void_arena_pop(NULL, scope);
        return ok;
    }
    const float winding_multiplier = area < 0.0f ? -1.0f : 1.0f;
    int vertices_left = count;
    int index_offset = 0;
    int current = 0;
    while (vertices_left > 3) {
        bool ear_found = false;
        int check = current;
        for (int i = 0; i < vertices_left; ++i) {
            const int p = prev[check];
            const int n = next[check];
            const float cp = cross_product(points[p], points[check], points[n]) * winding_multiplier;
            if (cp > 0.0f) {
                bool is_empty = true;
                int other = next[n];
                while (other != p) {
                    if (point_in_triangle(points[other], points[p], points[check], points[n])) {
                        is_empty = false;
                        break;
                    }
                    other = next[other];
                }
                if (is_empty) {
                    indices[index_offset++] = p;
                    indices[index_offset++] = check;
                    indices[index_offset++] = n;
                    next[p] = n;
                    prev[n] = p;
                    vertices_left--;
                    ear_found = true;
                    current = p;
                    break;
                }
            }
            check = next[check];
        }
        if (!ear_found) break;
    }
    if (vertices_left == 3) {
        const int n = next[current];
        indices[index_offset++] = prev[current];
        indices[index_offset++] = current;
        indices[index_offset++] = n;
    }
    const bool ok = SDL_RenderGeometry(window->render, NULL, vertices, count, indices, index_offset) == 0;
    void_arena_pop(NULL, scope);
    return ok;
}

// ============================================================================
// ELLIPSE, THICK LINE & ROUNDED RECTANGLE
// ============================================================================

bool void_gfx_draw_ellipse(const VoidWindow *window, const float x, const float y, const float radius_x, const float radius_y, const uint8 r, const uint8 g, const uint8 b, const uint8 a, const bool fill) {
    if (!window || !window->render || radius_x <= 0.0f || radius_y <= 0.0f) return false;
    const float max_radius = radius_x > radius_y ? radius_x : radius_y;
    int segments = (int)(max_radius * 1.5f);
    if (segments < 16) segments = 16;
    if (segments > 256) segments = 256;
    const float step = 2.0f * M_PIf / (float)segments;
    const VoidScope scope = void_arena_push(NULL);
    if (!fill) {
        VoidPoint *pts = void_arena_alloc(NULL, (segments + 1) * sizeof(VoidPoint), alignof(VoidPoint));
        if (!pts) {
            void_arena_pop(NULL, scope);
            return false;
        }
        for (int i = 0; i <= segments; ++i) {
            const float angle = (float)i * step;
            pts[i].x = x + cosf(angle) * radius_x;
            pts[i].y = y + sinf(angle) * radius_y;
        }
        const bool ok = void_render_lines(window, pts, segments + 1, r, g, b, a);
        void_arena_pop(NULL, scope);
        return ok;
    }
    const int num_vertices = segments + 1;
    const int num_indices = segments * 3;
    SDL_Vertex *vertices = void_arena_alloc(NULL, num_vertices * sizeof(SDL_Vertex), alignof(SDL_Vertex));
    int *indices = void_arena_alloc(NULL, num_indices * sizeof(int), alignof(int));
    if (!vertices || !indices) {
        void_arena_pop(NULL, scope);
        return false;
    }
    const SDL_Color col = {.r = r, .g = g, .b = b, .a = a};
    vertices[0].position.x = x;
    vertices[0].position.y = y;
    vertices[0].color = col;
    vertices[0].tex_coord.x = 0;
    vertices[0].tex_coord.y = 0;
    for (int i = 0; i < segments; ++i) {
        const float angle = (float)i * step;
        vertices[i + 1].position.x = x + cosf(angle) * radius_x;
        vertices[i + 1].position.y = y + sinf(angle) * radius_y;
        vertices[i + 1].color = col;
        vertices[i + 1].tex_coord.x = 0;
        vertices[i + 1].tex_coord.y = 0;
    }
    for (int i = 0; i < segments; ++i) {
        const int idx = i * 3;
        indices[idx] = 0;
        indices[idx + 1] = i + 1;
        indices[idx + 2] = (i + 1) % segments + 1;
    }
    const bool ok = SDL_RenderGeometry(window->render, NULL, vertices, num_vertices, indices, num_indices) == 0;
    void_arena_pop(NULL, scope);
    return ok;
}

bool void_gfx_draw_thick_line(const VoidWindow *window, const float x1, const float y1, const float x2, const float y2, const float thickness, const uint8 r, const uint8 g, const uint8 b, const uint8 a) {
    if (thickness <= 1.0f) {
        return void_render_line(window, x1, y1, x2, y2, r, g, b, a);
    }
    const float dx = x2 - x1;
    const float dy = y2 - y1;
    const float length = sqrtf(dx * dx + dy * dy);
    if (length == 0.0f) return false;
    const float nx = -dy / length * (thickness * 0.5f);
    const float ny = dx / length * (thickness * 0.5f);
    return void_gfx_draw_quad(window,
                              x1 + nx, y1 + ny,
                              x1 - nx, y1 - ny,
                              x2 - nx, y2 - ny,
                              x2 + nx, y2 + ny,
                              r, g, b, a, true);
}

bool void_gfx_draw_rounded_rect(const VoidWindow *window, const float x, const float y, const float w, const float h, float corner_radius, const uint8 r, const uint8 g, const uint8 b, const uint8 a, const bool fill) {
    if (corner_radius <= 0.0f) {
        return void_render_rect(window, x, y, w, h, r, g, b, a, fill);
    }
    const float min_dim = w < h ? w : h;
    if (corner_radius > min_dim * 0.5f) {
        corner_radius = min_dim * 0.5f;
    }
    const int corner_pts = 12;
    const int total_pts = corner_pts * 4;
    const VoidScope scope = void_arena_push(NULL);
    VoidPoint *pts = void_arena_alloc(NULL, total_pts * sizeof(VoidPoint), alignof(VoidPoint));
    if (!pts) {
        void_arena_pop(NULL, scope);
        return false;
    }
    const float c_x[4] = { x + w - corner_radius, x + corner_radius, x + corner_radius, x + w - corner_radius };
    const float c_y[4] = { y + h - corner_radius, y + h - corner_radius, y + corner_radius, y + corner_radius };
    const float start_angle[4] = { 0.0f, M_PIf * 0.5f, M_PIf, M_PIf * 1.5f };
    const float step = M_PIf * 0.5f / (float)(corner_pts - 1);
    int idx = 0;
    for (int corner = 0; corner < 4; ++corner) {
        for (int i = 0; i < corner_pts; ++i) {
            const float angle = start_angle[corner] + (float)i * step;
            pts[idx].x = c_x[corner] + cosf(angle) * corner_radius;
            pts[idx].y = c_y[corner] + sinf(angle) * corner_radius;
            idx++;
        }
    }
    const bool ok = void_gfx_draw_polygon(window, pts, total_pts, r, g, b, a, fill);
    void_arena_pop(NULL, scope);
    return ok;
}

// ============================================================================
// PIE, BEZIER, RING & CAPSULE
// ============================================================================

bool void_gfx_draw_pie(const VoidWindow *window, const float x, const float y, const float radius, const float start_angle, const float end_angle, const uint8 r, const uint8 g, const uint8 b, const uint8 a, const bool fill) {
    if (!window || !window->render || radius <= 0.0f) return false;
    float angle_diff = end_angle - start_angle;
    while (angle_diff < 0.0f) angle_diff += M_PIf * 2.0f;
    if (angle_diff > M_PIf * 2.0f) angle_diff = M_PIf * 2.0f;
    int segments = (int)(radius * angle_diff * 0.25f);
    if (segments < 8) segments = 8;
    if (segments > 256) segments = 256;
    const float step = angle_diff / (float)segments;
    const VoidScope scope = void_arena_push(NULL);
    if (!fill) {
        VoidPoint *pts = void_arena_alloc(NULL, (segments + 2) * sizeof(VoidPoint), alignof(VoidPoint));
        if (!pts) {
            void_arena_pop(NULL, scope);
            return false;
        }
        pts[0].x = x;
        pts[0].y = y;
        for (int i = 0; i <= segments; ++i) {
            const float current_angle = start_angle + (float)i * step;
            pts[i + 1].x = x + cosf(current_angle) * radius;
            pts[i + 1].y = y + sinf(current_angle) * radius;
        }
        const bool ok = void_render_lines(window, pts, segments + 2, r, g, b, a);
        void_arena_pop(NULL, scope);
        return ok;
    }
    const int num_vertices = segments + 2;
    const int num_indices = segments * 3;
    SDL_Vertex *vertices = void_arena_alloc(NULL, num_vertices * sizeof(SDL_Vertex), alignof(SDL_Vertex));
    int *indices = void_arena_alloc(NULL, num_indices * sizeof(int), alignof(int));
    if (!vertices || !indices) {
        void_arena_pop(NULL, scope);
        return false;
    }
    const SDL_Color col = {.r = r, .g = g, .b = b, .a = a};
    vertices[0].position.x = x;
    vertices[0].position.y = y;
    vertices[0].color = col;
    vertices[0].tex_coord.x = 0;
    vertices[0].tex_coord.y = 0;
    for (int i = 0; i <= segments; ++i) {
        const float current_angle = start_angle + (float)i * step;
        vertices[i + 1].position.x = x + cosf(current_angle) * radius;
        vertices[i + 1].position.y = y + sinf(current_angle) * radius;
        vertices[i + 1].color = col;
        vertices[i + 1].tex_coord.x = 0;
        vertices[i + 1].tex_coord.y = 0;
    }
    for (int i = 0; i < segments; ++i) {
        const int idx = i * 3;
        indices[idx] = 0;
        indices[idx + 1] = i + 1;
        indices[idx + 2] = i + 2;
    }
    const bool ok = SDL_RenderGeometry(window->render, NULL, vertices, num_vertices, indices, num_indices) == 0;
    void_arena_pop(NULL, scope);
    return ok;
}

bool void_gfx_draw_bezier(const VoidWindow *window, const float x1, const float y1, const float cx, const float cy, const float x2, const float y2, int segments, const uint8 r, const uint8 g, const uint8 b, const uint8 a) {
    if (!window || !window->render) return false;
    if (segments < 4) segments = 4;
    if (segments > 256) segments = 256;
    const VoidScope scope = void_arena_push(NULL);
    VoidPoint *pts = void_arena_alloc(NULL, (segments + 1) * sizeof(VoidPoint), alignof(VoidPoint));
    if (!pts) {
        void_arena_pop(NULL, scope);
        return false;
    }
    for (int i = 0; i <= segments; ++i) {
        const float t = (float)i / (float)segments;
        const float u = 1.0f - t;
        const float tt = t * t;
        const float uu = u * u;
        const float ut2 = 2.0f * u * t;
        pts[i].x = uu * x1 + ut2 * cx + tt * x2;
        pts[i].y = uu * y1 + ut2 * cy + tt * y2;
    }
    const bool ok = void_render_lines(window, pts, segments + 1, r, g, b, a);
    void_arena_pop(NULL, scope);
    return ok;
}

bool void_gfx_draw_ring(const VoidWindow *window, const float x, const float y, const float inner_radius, const float outer_radius, const uint8 r, const uint8 g, const uint8 b, const uint8 a, const bool fill) {
    if (!window || !window->render || inner_radius <= 0.0f || outer_radius <= inner_radius) return false;
    int segments = (int)(outer_radius * 1.5f);
    if (segments < 16) segments = 16;
    if (segments > 256) segments = 256;
    const float step = 2.0f * M_PIf / (float)segments;
    if (!fill) {
        const bool ok = void_gfx_draw_circle(window, x, y, inner_radius, r, g, b, a, false);
        return ok && void_gfx_draw_circle(window, x, y, outer_radius, r, g, b, a, false);
    }
    const int num_vertices = segments * 2;
    const int num_indices = segments * 6;
    const VoidScope scope = void_arena_push(NULL);
    SDL_Vertex *vertices = void_arena_alloc(NULL, num_vertices * sizeof(SDL_Vertex), alignof(SDL_Vertex));
    int *indices = void_arena_alloc(NULL, num_indices * sizeof(int), alignof(int));
    if (!vertices || !indices) {
        void_arena_pop(NULL, scope);
        return false;
    }
    const SDL_Color col = {.r = r, .g = g, .b = b, .a = a};
    for (int i = 0; i < segments; ++i) {
        const float angle = (float)i * step;
        const float c = cosf(angle);
        const float s = sinf(angle);
        vertices[i * 2].position.x = x + c * inner_radius;
        vertices[i * 2].position.y = y + s * inner_radius;
        vertices[i * 2].color = col;
        vertices[i * 2].tex_coord.x = 0;
        vertices[i * 2].tex_coord.y = 0;
        vertices[i * 2 + 1].position.x = x + c * outer_radius;
        vertices[i * 2 + 1].position.y = y + s * outer_radius;
        vertices[i * 2 + 1].color = col;
        vertices[i * 2 + 1].tex_coord.x = 0;
        vertices[i * 2 + 1].tex_coord.y = 0;
    }
    for (int i = 0; i < segments; ++i) {
        const int idx = i * 6;
        const int current_inner = i * 2;
        const int current_outer = i * 2 + 1;
        const int next_inner = ((i + 1) % segments) * 2;
        const int next_outer = ((i + 1) % segments) * 2 + 1;
        indices[idx] = current_inner;
        indices[idx + 1] = current_outer;
        indices[idx + 2] = next_outer;
        indices[idx + 3] = current_inner;
        indices[idx + 4] = next_outer;
        indices[idx + 5] = next_inner;
    }
    const bool ok = SDL_RenderGeometry(window->render, NULL, vertices, num_vertices, indices, num_indices) == 0;
    void_arena_pop(NULL, scope);
    return ok;
}

bool void_gfx_draw_capsule(const VoidWindow *window, const float x, const float y, const float w, const float h, const uint8 r, const uint8 g, const uint8 b, const uint8 a, const bool fill) {
    const float min_dim = w < h ? w : h;
    return void_gfx_draw_rounded_rect(window, x, y, w, h, min_dim * 0.5f, r, g, b, a, fill);
}

// ============================================================================
// TEXTURED GEOMETRY
// ============================================================================

bool void_gfx_draw_textured_quad(const VoidWindow *window, const VoidTexture *texture, const VoidPoint *pts, const VoidPoint *uvs, const uint8 r, const uint8 g, const uint8 b, const uint8 a) {
    if (!window || !window->render || !texture || !texture->handle || !pts || !uvs) return false;
    const SDL_Color col = {.r = r, .g = g, .b = b, .a = a};
    const SDL_Vertex vertices[4] = {
        {.position = {.x = pts[0].x, .y = pts[0].y}, .color = col, .tex_coord = {.x = uvs[0].x, .y = uvs[0].y}},
        {.position = {.x = pts[1].x, .y = pts[1].y}, .color = col, .tex_coord = {.x = uvs[1].x, .y = uvs[1].y}},
        {.position = {.x = pts[2].x, .y = pts[2].y}, .color = col, .tex_coord = {.x = uvs[2].x, .y = uvs[2].y}},
        {.position = {.x = pts[3].x, .y = pts[3].y}, .color = col, .tex_coord = {.x = uvs[3].x, .y = uvs[3].y}}
    };
    const int indices[6] = {0, 1, 2, 0, 2, 3};
    return SDL_RenderGeometry(window->render, texture->handle, vertices, 4, indices, 6) == 0;
}