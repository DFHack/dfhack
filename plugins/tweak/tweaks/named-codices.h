// Show name for titled books

#include "df/artifact_record.h"
#include "df/general_ref.h"
#include "df/item_bookst.h"

struct book_hook : df::item_bookst {
    typedef df::item_bookst interpose_base;
    DEFINE_VMETHOD_INTERPOSE(void, getItemDescription, (std::string *str, int8_t plurality)) {
        if (plurality) {
            // plurality is set when creating the long description, where we don't want to
            // replace the string with the title
            INTERPOSE_NEXT(getItemDescription)(str, plurality);
            return;
        }
        if (this->flags.bits.artifact) {
            if (auto gref = Items::getGeneralRef(this, df::general_ref_type::IS_ARTIFACT)) {
                if (auto artifact = gref->getArtifact()) {
                    if (!artifact->name.has_name) {
                        // this part could potentially be done in a periodic scan-and-fix
                        artifact->name.first_name = Items::getBookTitle(this);
                        if (artifact->name.first_name.size()) {
                            artifact->name.has_name = true;
                        }
                    }
                    if (!artifact->name.has_name) {
                        str->append("Untitled");
                        return;
                    }
                }
            }
        } else if (this->hasSpecificImprovements(df::improvement_type::PAGES)
                || this->hasSpecificImprovements(df::improvement_type::WRITING)) {
            auto title = Items::getBookTitle(this);
            if (title.size())
                str->append(title);
            else
                str->append("Untitled");
            return;
        }
        INTERPOSE_NEXT(getItemDescription)(str, plurality);
    }
};
IMPLEMENT_VMETHOD_INTERPOSE(book_hook, getItemDescription);
