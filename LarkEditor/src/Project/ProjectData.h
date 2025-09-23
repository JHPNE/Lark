class ProjectData : public ISerializable
{
  public:
    std::string name;
    fs::path path;
    std::string date;

    fs::path GetFullPath() const { return path / (name + Project::Extension); }

    void Serialize(tinyxml2::XMLElement *element, SerializationContext &context) const override
    {
        SerializerUtils::WriteElement(context.document, element, "Date", date);
        SerializerUtils::WriteElement(context.document, element, "ProjectName", name);
        SerializerUtils::WriteElement(context.document, element, "ProjectPath", path.string());
    }

    bool Deserialize(const tinyxml2::XMLElement *element, SerializationContext &context) override
    {
        std::string pathStr;
        if (!SerializerUtils::ReadElement(element, "Date", date) ||
            !SerializerUtils::ReadElement(element, "ProjectName", name) ||
            !SerializerUtils::ReadElement(element, "ProjectPath", pathStr))
        {
            return false;
        }

        path = fs::path(pathStr);

        // Log for debugging
        Logger::Get().Log(MessageType::Info, "Deserialized ProjectData - Name: " + name +
                                                 ", Path: " + path.string() + ", Date: " + date);

        return true;
    }

    Version GetVersion() const override { return {1, 0, 0}; }

    bool operator==(const ProjectData& other) const {
        return name == other.name &&
               path == other.path &&
               date == other.date;
    }

    bool operator!=(const ProjectData& other) const {
        return !(*this == other);
    }
};