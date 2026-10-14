// Composable file filter ("find"-like). Build check: g++ -std=c++17 -Wall -Wextra -c find_lib.cpp
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

struct FileInfo {
    std::string path;
    std::string extension;      // lower case, without the dot: "pdf"
    std::uint64_t size_bytes;
};

constexpr std::uint64_t kMB = 1024ull * 1024ull;

// ---- 1. One small interface: "does this file match?" -------------------------------------
class Criterion {
public:
    virtual ~Criterion() = default;
    virtual bool matches(const FileInfo& file) const = 0;
};

// ---- 2. Value-type handle, so criteria can be combined with & | ! -------------------------
class Filter {
public:
    explicit Filter(std::shared_ptr<const Criterion> criterion) : criterion_(std::move(criterion)) {}
    bool matches(const FileInfo& file) const { return criterion_->matches(file); }
private:
    std::shared_ptr<const Criterion> criterion_;
};

// ---- 3. Combinators: they only know the Criterion interface, never the concrete criteria --
class AndCriterion : public Criterion {
public:
    AndCriterion(Filter left, Filter right) : left_(std::move(left)), right_(std::move(right)) {}
    bool matches(const FileInfo& f) const override { return left_.matches(f) && right_.matches(f); }
private:
    Filter left_, right_;
};

class OrCriterion : public Criterion {
public:
    OrCriterion(Filter left, Filter right) : left_(std::move(left)), right_(std::move(right)) {}
    bool matches(const FileInfo& f) const override { return left_.matches(f) || right_.matches(f); }
private:
    Filter left_, right_;
};

class NotCriterion : public Criterion {
public:
    explicit NotCriterion(Filter inner) : inner_(std::move(inner)) {}
    bool matches(const FileInfo& f) const override { return !inner_.matches(f); }
private:
    Filter inner_;
};

Filter operator&(Filter a, Filter b) { return Filter(std::make_shared<AndCriterion>(std::move(a), std::move(b))); }
Filter operator|(Filter a, Filter b) { return Filter(std::make_shared<OrCriterion>(std::move(a), std::move(b))); }
Filter operator!(Filter a)           { return Filter(std::make_shared<NotCriterion>(std::move(a))); }

// ---- 4. Leaf criteria: the two seed cases ------------------------------------------------
class SizeGreaterThan : public Criterion {
public:
    explicit SizeGreaterThan(std::uint64_t bytes) : bytes_(bytes) {}
    bool matches(const FileInfo& f) const override { return f.size_bytes > bytes_; }
private:
    std::uint64_t bytes_;
};

class ExtensionIs : public Criterion {
public:
    explicit ExtensionIs(std::string ext) : ext_(to_lower(std::move(ext))) {}
    bool matches(const FileInfo& f) const override { return f.extension == ext_; }
private:
    static std::string to_lower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        return s;
    }
    std::string ext_;
};

// Readable factory functions: this is what the caller writes.
Filter size_greater_than(std::uint64_t bytes) { return Filter(std::make_shared<SizeGreaterThan>(bytes)); }
Filter extension_is(std::string ext)          { return Filter(std::make_shared<ExtensionIs>(std::move(ext))); }

// ---- 5. The search itself knows nothing about criteria types ------------------------------
std::vector<FileInfo> find(const std::vector<FileInfo>& files, const Filter& filter)
{
    std::vector<FileInfo> result;
    std::copy_if(files.begin(), files.end(), std::back_inserter(result),
                 [&filter](const FileInfo& f) { return filter.matches(f); });
    return result;
}

// Usage:
//   auto big       = size_greater_than(5 * kMB);
//   auto xml       = extension_is("xml");
//   auto complex   = (extension_is("pdf") & size_greater_than(5 * kMB))
//                  | (extension_is("docx") & size_greater_than(3 * kMB));
//   auto matches   = find(all_files, complex);
//
// Adding a new criterion (e.g. NameContains) = one new class + one factory function.
// Nothing above changes.
