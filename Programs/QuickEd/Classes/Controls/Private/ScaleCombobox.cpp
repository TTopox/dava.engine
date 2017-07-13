#include "Controls/ScaleCombobox.h"

#include <TArc/Utils/ScopedValueGuard.h>
#include <TArc/DataProcessing/AnyQMetaType.h>

#include <Base/FastName.h>
#include <Logger/Logger.h>
#include <Reflection/ReflectedMeta.h>

#include <QLineEdit>
#include <QSignalBlocker>
#include <QVariant>

ScaleComboBox::ScaleComboBox(const Params& params, DAVA::TArc::ContextAccessor* accessor, DAVA::Reflection model, QWidget* parent)
    : ControlProxyImpl<QComboBox>(params, DAVA::TArc::ControlDescriptor(params.fields), accessor, model, parent)
{
    SetupControl();
}

void ScaleComboBox::SetupControl()
{
    setEditable(true);

    connections.AddConnection(this, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), DAVA::MakeFunction(this, &ScaleComboBox::CurrentIndexChanged));
    connections.AddConnection(lineEdit(), &QLineEdit::editingFinished, DAVA::MakeFunction(this, &ScaleComboBox::EditingFinished));
    setSizeAdjustPolicy(QComboBox::AdjustToContents);

    setInsertPolicy(QComboBox::NoInsert);
    setEnabled(count() > 0);
}

void ScaleComboBox::UpdateControl(const DAVA::TArc::ControlDescriptor& changedFields)
{
    using namespace DAVA;
    using namespace DAVA::TArc;

    DVASSERT(updateControlProceed == false);
    ScopedValueGuard<bool> guard(updateControlProceed, true);

    if (changedFields.IsChanged(Fields::Enabled))
    {
        setEnabled(GetFieldValue<bool>(Fields::Enabled, false));
    }

    if (changedFields.IsChanged(Fields::Enumerator))
    {
        CreateItems(model.GetField(changedFields.GetName(Fields::Enumerator)));
    }

    Any value = model.GetField(changedFields.GetName(Fields::Value)).GetValue();
    SetCurrentValue(value);

    UpdateValidator();
}

void ScaleComboBox::CreateItems(const DAVA::Reflection& fieldEnumerator)
{
    using namespace DAVA;

    QSignalBlocker blockSignals(this);

    clear();
    Vector<Reflection::Field> fields = fieldEnumerator.GetFields();
    for (Reflection::Field& field : fields)
    {
        Any fieldDescr = field.ref.GetValue();

        DVASSERT(fieldDescr.CanCast<float32>());
        float32 value = fieldDescr.Cast<float32>();

        addItem(ValueToString(value), value);
    }
}

void ScaleComboBox::SetCurrentValue(const DAVA::Any& value)
{
    using namespace DAVA;

    QSignalBlocker blocker(this);
    if (value.CanGet<float32>())
    {
        float32 fltValue = value.Get<float32>();
        int index = findData(QVariant(fltValue));
        if (index != -1)
        {
            setCurrentIndex(index);
        }

        lineEdit()->setText(ValueToString(fltValue));
    }
    else
    {
        lineEdit()->setText(QString());
    }
}

void ScaleComboBox::CurrentIndexChanged(int newCurrentItem)
{
    if (updateControlProceed)
    {
        // ignore reaction on control initialization
        return;
    }

    wrapper.SetFieldValue(GetFieldName(Fields::Value), StringToValue(currentText()));
}

void ScaleComboBox::EditingFinished()
{
    if (updateControlProceed)
    {
        // ignore reaction on control initialization
        return;
    }

    wrapper.SetFieldValue(GetFieldName(Fields::Value), StringToValue(currentText()));
}

QString ScaleComboBox::ValueToString(DAVA::float32 value) const
{
    return QString("%1").arg(static_cast<int>(value * 100.0f + 0.5f));
}

DAVA::float32 ScaleComboBox::StringToValue(const QString& text) const
{
    QString curTextValue = currentText();

    bool ok;
    float value = curTextValue.toFloat(&ok);
    DVASSERT(ok, "can not parse text to float");
    return value / 100.0f;
}

void ScaleComboBox::UpdateValidator()
{
    using namespace DAVA;
    if (count() > 0)
    {
        float32 first = itemData(0).value<float32>();
        float32 last = itemData(count() - 1).value<float32>();
        delete validator();
        setValidator(new QIntValidator(first * 100, last * 100));
    }
}
