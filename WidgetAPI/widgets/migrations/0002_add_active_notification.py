# Generated manually for ActiveNotification

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('widgets', '0001_initial'),
    ]

    operations = [
        migrations.CreateModel(
            name='ActiveNotification',
            fields=[
                ('id', models.BigAutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('title', models.CharField(max_length=128)),
                ('message', models.TextField()),
                ('level', models.CharField(choices=[('INFO', 'Info'), ('WARNING', 'Warning'), ('CRITICAL', 'Critical')], default='CRITICAL', max_length=16)),
                ('is_active', models.BooleanField(default=True)),
                ('sound_enabled', models.BooleanField(default=True)),
                ('volume', models.PositiveSmallIntegerField(default=100, help_text='0-100')),
                ('duration_ms', models.PositiveIntegerField(default=500)),
                ('frequency', models.PositiveIntegerField(default=2500, help_text='Hz')),
                ('created_at', models.DateTimeField(auto_now_add=True)),
                ('updated_at', models.DateTimeField(auto_now=True)),
                ('source_identifier', models.CharField(blank=True, db_index=True, default='', max_length=64)),
            ],
            options={
                'verbose_name': 'Active notification',
                'verbose_name_plural': 'Active notifications',
                'ordering': ['-created_at'],
            },
        ),
    ]
