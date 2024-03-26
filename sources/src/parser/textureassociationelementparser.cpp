#include "parser/textureassociationelementparser.h"

#include <unordered_map>

#include "parser/nodetypes.h"
#include "parser/attributes.h"
#include "parser/documentlocation.h"
#include "parser/parserutils.hpp"
#include "parser/skipelementparser.h"

#include <citygml/cityobject.h>
#include <citygml/citygmlfactory.h>
#include <citygml/citygmllogger.h>
#include <citygml/texture.h>
#include <citygml/texturetargetdefinition.h>
#include <citygml/texturecoordinates.h>

#include <stdexcept>

namespace citygml {

    TextureAssociationElementParser::TextureAssociationElementParser(CityGMLDocumentParser& documentParser, CityGMLFactory& factory, std::shared_ptr<CityGMLLogger> logger, std::shared_ptr<Texture> texture)
        : GMLObjectElementParser(documentParser, factory, logger)
    {
        m_model = texture;
        m_currentTexCoords = nullptr;
    }

    std::string TextureAssociationElementParser::elementParserName() const
    {
        return "TextureAssociationElementParser";
    }

    bool TextureAssociationElementParser::handlesElement(const NodeType::XMLNode& node) const
    {
        return node == NodeType::APP_TextureAssociationNode;
    }

    bool TextureAssociationElementParser::parseElementStartTag(const NodeType::XMLNode& node, Attributes& attributes)
    {
        if (node != NodeType::APP_TextureAssociationNode) {
            CITYGML_LOG_ERROR(m_logger, "Expected start tag <" << NodeType::APP_TextureAssociationNode.name() << "> got " << node << " at " << getDocumentLocation());
            throw std::runtime_error("Unexpected start tag found.");
        }

        return true;
    }

    bool TextureAssociationElementParser::parseElementEndTag(const NodeType::XMLNode&, const std::string&)
    {
        m_currentTexTargetDef = m_factory.createTextureTargetDefinition(parseReference(m_currentTargetUri, m_logger, getDocumentLocation()), m_model, m_currentGmlId);
        if (m_currentTexCoords != nullptr) {
            m_currentTexTargetDef->addTexCoordinates(m_currentTexCoords);
        }
        return true;
    }

    bool TextureAssociationElementParser::parseChildElementStartTag(const NodeType::XMLNode& node, Attributes& attributes)
    {
        if (m_model == nullptr) {
            throw std::runtime_error("TextureAssociationElementParser::parseChildElementStartTag called before TextureAssociationElementParser::parseElementStartTag");
        }

        if (node == NodeType::APP_TextureParameterizationNode
            || node == NodeType::APP_TexCoordListNode
            || node == NodeType::APP_IsFrontNode
            || node == NodeType::APP_MimeTypeNode) {
            return true;
        } else if (node == NodeType::APP_TargetNode) {
            if (!m_currentTargetUri.empty()) {
                CITYGML_LOG_WARN(m_logger, "Multipl etexture target definitions detected at: " << getDocumentLocation());
            }
            m_currentGmlId = attributes.getCityGMLIDAttribute();
            return true;
        } else if (node == NodeType::APP_TextureCoordinatesNode) {
            if (m_currentTexCoords != nullptr) {
                CITYGML_LOG_WARN(m_logger, "Nested texture coordinates definition detected at: " << getDocumentLocation());
            } else {
                m_currentTexCoords = std::make_shared<TextureCoordinates>(attributes.getCityGMLIDAttribute(), parseReference(attributes.getAttribute("ring"), m_logger, getDocumentLocation()));
            }
            return true;
        }

        return GMLObjectElementParser::parseChildElementStartTag(node, attributes);
    }

    bool TextureAssociationElementParser::parseChildElementEndTag(const NodeType::XMLNode& node, const std::string& characters)
    {
        if (m_model == nullptr) {
            throw std::runtime_error("TextureAssociationElementParser::parseChildElementEndTag called before TextureAssociationElementParser::parseElementStartTag");
        }

        if (node == NodeType::APP_TextureParameterizationNode) {
            // Do nothing (target and texture coords are set in child element)
        } else if (node == NodeType::APP_TexCoordListNode) {

            if (m_currentTexCoords != nullptr) {
                CITYGML_LOG_WARN(m_logger, "TexCoordList node finished before TextureCoordinates child is finished at " << getDocumentLocation());
            }

        } else if (node == NodeType::APP_TextureCoordinatesNode) {

            if (m_currentTexCoords != nullptr) {
                m_currentTexCoords->setCoords(parseVecList<TVec2f>(characters, m_logger, getDocumentLocation()));
            } else {
                CITYGML_LOG_WARN(m_logger, "Unexpected end tag <" << NodeType::APP_TextureCoordinatesNode << " at: " << getDocumentLocation());
            }
        } else if (node == NodeType::APP_TargetNode) {
            m_currentTargetUri = parseReference(characters, m_logger, getDocumentLocation());
        } else if (node == NodeType::APP_MimeTypeNode) {
            m_model->setAttribute(node.name(), characters);
        } else {
            return GMLObjectElementParser::parseChildElementEndTag(node, characters);
        }
        return true;
    }

    Object* TextureAssociationElementParser::getObject()
    {
        return m_model.get();
    }
}
