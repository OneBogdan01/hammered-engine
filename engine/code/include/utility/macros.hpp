
#define HM_NON_COPYABLE_NON_MOVABLE(ClassName)     \
  ClassName(const ClassName&) = delete;            \
  ClassName& operator=(const ClassName&) = delete; \
  ClassName(ClassName&&) = delete;                 \
  ClassName& operator=(ClassName&&) = delete

#define HM_VIRTUAL_BASE_CLASS(ClassName)                \
 public:                                                \
  virtual ~ClassName() = default;                       \
  ClassName(const ClassName&) = delete;                 \
  ClassName& operator=(const ClassName&) = delete;      \
  ClassName(ClassName&&) noexcept = default;            \
  ClassName& operator=(ClassName&&) noexcept = default; \
                                                        \
 protected:                                             \
  ClassName() = default

#define HM_NON_COPYABLE(ClassName)                 \
  ClassName(const ClassName&) = delete;            \
  ClassName& operator=(const ClassName&) = delete; \
  ClassName(ClassName&&) noexcept = default;       \
  ClassName& operator=(ClassName&&) noexcept = default

#define HM_DELETE_COPY(ClassName)       \
  ClassName(const ClassName&) = delete; \
  ClassName& operator=(const ClassName&) = delete

#define HM_DELETE_MOVE(ClassName)  \
  ClassName(ClassName&&) = delete; \
  ClassName& operator=(ClassName&&) = delete

#define HM_DEFAULT_MOVE(ClassName)           \
  ClassName(ClassName&&) noexcept = default; \
  ClassName& operator=(ClassName&&) noexcept = default
